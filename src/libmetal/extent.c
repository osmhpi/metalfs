#include <lmdb.h>

#include <stddef.h>
#include <assert.h>

#include "metal.h"
#include "hollow_heap.h"

#include "extent.h"

#define EXTENTS_DB_NAME "extents"

MDB_dbi extents_db = 0;

typedef enum mtl_extent_status {
    MTL_FREE,
    MTL_RESERVED,
    MTL_COMMITTED
} mtl_extent_status;

typedef struct mtl_extent {
    uint64_t length;
    mtl_extent_status status;
    mtl_heap_node_id pq_node;
} mtl_extent;

int mtl_ensure_extents_db_open(MDB_txn *txn) {
    if (extents_db == 0)
        mdb_dbi_open(txn, EXTENTS_DB_NAME, MDB_CREATE, &extents_db);
    return MTL_SUCCESS;
}

int mtl_load_extent(MDB_txn *txn, uint64_t offset, const mtl_extent **extent) {

    MDB_val extent_key = { .mv_size = sizeof(offset), .mv_data = &offset };
    MDB_val extent_value;

    if (mdb_get(txn, extents_db, &extent_key, &extent_value) == MDB_NOTFOUND)
        return MTL_ERROR_NOENTRY;

    if (extent) {
        *extent = extent_value.mv_data;
    }

    return MTL_SUCCESS;
}

int mtl_put_extent(MDB_txn *txn, uint64_t offset, mtl_extent *extent) {

    MDB_val extent_key = { .mv_size = sizeof(offset), .mv_data = &offset };
    MDB_val extent_value = { .mv_size = sizeof(*extent), .mv_data = extent };
    mdb_put(txn, extents_db, &extent_key, &extent_value, 0);

    return MTL_SUCCESS;
}

int mtl_initialize_extents(MDB_txn *txn, uint64_t blocks) {

    mtl_ensure_extents_db_open(txn);

    const mtl_extent *first_extent;
    if (mtl_load_extent(txn, 0, &first_extent) == MTL_ERROR_NOENTRY) {
        mtl_extent all_extent = { .length = blocks, .status = MTL_FREE };
        mtl_heap_insert(txn, blocks, 0, &all_extent.pq_node);
        mtl_put_extent(txn, 0, &all_extent);
    }

    return MTL_SUCCESS;
}

uint64_t mtl_reserve_extent(MDB_txn *txn, uint64_t size, uint64_t *offset) {

    mtl_ensure_extents_db_open(txn);

    int res;

    // Pop from heap
    uint64_t extent_offset;
    if (mtl_heap_extract_max(txn, &extent_offset) == MTL_ERROR_NOENTRY) {
        return 0;
    }

    // Load extent
    const mtl_extent *extent;
    res = mtl_load_extent(txn, extent_offset, &extent);
    assert(res == MTL_SUCCESS);

    uint64_t extent_length = extent->length;

    mtl_extent updated_extent = *extent;

    // Split up if necessary
    if (extent_length > size) {
        uint64_t remaining_extent_offset = extent_offset + size;
        uint64_t remaining_extent_length = extent_length - size;
        mtl_extent remaining_extent = { .length = remaining_extent_length, .status = MTL_FREE };

        // Insert remaining extent into heap
        mtl_heap_insert(txn, remaining_extent_length, remaining_extent_offset, &remaining_extent.pq_node);

        mtl_put_extent(txn, remaining_extent_offset, &remaining_extent);

        // Update length of existing extent
        updated_extent.length = size;
        extent_length = size;
    }

    // Update status of existing extent
    updated_extent.status = MTL_RESERVED;
    mtl_put_extent(txn, extent_offset, &updated_extent);

    // Return offset and size
    if (offset)
        *offset = extent_offset;

    return extent_length;
}

int mtl_commit_extent(MDB_txn *txn, uint64_t offset, uint64_t len) {

    mtl_ensure_extents_db_open(txn);

    // Find extent by offset
    const mtl_extent *extent;
    mtl_load_extent(txn, offset, &extent);

    // Update status
    mtl_extent updated_extent = *extent;
    updated_extent.status = MTL_COMMITTED;
    mtl_put_extent(txn, offset, &updated_extent);
    return MTL_SUCCESS;
}

int mtl_free_extent(MDB_txn *txn, uint64_t offset) {

    mtl_ensure_extents_db_open(txn);

    int res;

    // Find extent by offset
    const mtl_extent *extent;
    mtl_load_extent(txn, offset, &extent);
    uint64_t extent_offset = offset;
    uint64_t extent_size = extent->length;
    mtl_heap_node_id pq_node = INVALID_NODE;

    MDB_cursor *cursor;
    mdb_cursor_open(txn, extents_db, &cursor);

    MDB_val extent_key = { .mv_size = sizeof(offset), .mv_data = &offset };

    // Check if the following extent is also free
    {
        MDB_val next_extent_key;
        MDB_val next_extent_value;
        mdb_cursor_get(cursor, &extent_key, &next_extent_value, MDB_SET);
        res = mdb_cursor_get(cursor, &next_extent_key, &next_extent_value, MDB_NEXT);

        // uint64_t next_extent_offset = res == MDB_NOTFOUND ? 0 : *(uint64_t*)next_extent_key.mv_data;
        mtl_extent *next_extent = res == MDB_NOTFOUND ? NULL : next_extent_value.mv_data;

        if (next_extent && next_extent->status == MTL_FREE) {
            extent_size += next_extent->length;
            // Remove from heap
           mtl_heap_delete(txn, next_extent->pq_node);

            // Remove from extents
            mdb_cursor_del(cursor, 0);
        }
    }

    // Check if we can merge with the previous extent
    {
        MDB_val previous_extent_key;
        MDB_val previous_extent_value;
        mdb_cursor_get(cursor, &extent_key, &previous_extent_value, MDB_SET);
        res = mdb_cursor_get(cursor, &previous_extent_key, &previous_extent_value, MDB_NEXT);

        uint64_t previous_extent_offset = res == MDB_NOTFOUND ? 0 : *(uint64_t*)previous_extent_key.mv_data;
        mtl_extent *previous_extent = res == MDB_NOTFOUND ? NULL : previous_extent_value.mv_data;

        if (previous_extent && previous_extent->status == MTL_FREE) {
            extent_offset = previous_extent_offset;
            extent_size += previous_extent->length;
            pq_node = previous_extent->pq_node;

            // Remove this extent
            mdb_del(txn, extents_db, &extent_key, NULL);
        }
    }

    mdb_cursor_close(cursor);

    // Insert resulting extent into heap
    if (pq_node != INVALID_NODE) {
        mtl_heap_increase_key(txn, pq_node, extent_size, &pq_node);
    } else {
        mtl_heap_insert(txn, extent_size, extent_offset, &pq_node);
    }

    // Create new extent
    mtl_extent updated_extent = {
        .status = MTL_FREE,
        .length = extent_size,
        .pq_node = pq_node
    };
    
    MDB_val updated_extent_key = { .mv_size = sizeof(extent_offset), .mv_data = &extent_offset };
    MDB_val updated_extent_value = { .mv_size = sizeof(updated_extent), .mv_data = &updated_extent };
    mdb_put(txn, extents_db, &updated_extent_key, &updated_extent_value, 0);

    return MTL_SUCCESS;
}

int mtl_reset_extents_db() {
    extents_db = 0;
    return MTL_SUCCESS;
}
