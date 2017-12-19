#include <lmdb.h>

#include <stddef.h>

#include "metal.h"

#include "extent.h"

#define EXTENTS_DB_NAME "extents"
#define INVALID_EXTENT (1 << 63)

MDB_dbi extents_db = 0;

typedef struct mtl_hollow_heap {
    uint64_t root;
    uint64_t size;
} mtl_hollow_heap;

int mtl_ensure_extents_db_open(MDB_txn *txn) {
    if (extents_db == 0)
        mdb_dbi_open(txn, EXTENTS_DB_NAME, MDB_CREATE, &extents_db);
    return 0;
}

uint64_t mtl_reserve_extent(uint64_t _size, uint64_t *offset) {
    // Pop from heap
    // Split up if necessary
    // Insert remaning extent into heap
    // Return offset and size
    return 0;
}

int mtl_commit_extent(uint64_t offset, uint64_t len) {
    // Find extent by offset
    // Update status
    return 0;
}

int mtl_free_extent(uint64_t offset) {
    // Find extent by offset
    // Update status
    // Find previous and next extent
    // Merge if possible
    // Insert resulting extent into heap
    return 0;
}

int mtl_reset_extents_db() {
    extents_db = 0;
    return MTL_SUCCESS;
}
