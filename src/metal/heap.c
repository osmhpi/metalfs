#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include "metal.h"
#include "meta.h"

#include "heap.h"

#define HEAP_DB_NAME "heap"

mtl_heap_node_id INVALID_NODE = { 0, 0 };

MDB_dbi heap_db = 0;

int mtl_ensure_heap_db_open(MDB_txn *txn) {
    if (heap_db == 0)
        mdb_dbi_open(txn, HEAP_DB_NAME, MDB_CREATE, &heap_db);
    return 0;
}

int mtl_heap_insert(MDB_txn *txn, uint64_t key, uint64_t value, mtl_heap_node_id *node_id) {

    mtl_ensure_heap_db_open(txn);

    mtl_heap_node_id id = { .k=key, .v=value };

    MDB_val k = { .mv_size = sizeof(*node_id), .mv_data = &id };
    MDB_val v = { .mv_size = sizeof(uint64_t), .mv_data = &value };
    mdb_put(txn, heap_db, &k, &v, 0);

    if (node_id)
        *node_id = id;

    return MTL_SUCCESS;
}

int mtl_heap_extract_max(MDB_txn *txn, uint64_t *max_value) {

    mtl_ensure_heap_db_open(txn);

    MDB_cursor *cursor;
    mdb_cursor_open(txn, heap_db, &cursor);

    MDB_val key;
    MDB_val value;
    mdb_cursor_get(cursor, &key, &value, MDB_LAST);

    mdb_cursor_close(cursor);

    *max_value = *(uint64_t*)value.mv_data;

    return mtl_heap_delete(txn, *(mtl_heap_node_id*)key.mv_data);
}

int mtl_heap_delete(MDB_txn *txn, mtl_heap_node_id node_id) {

    mtl_ensure_heap_db_open(txn);

    MDB_val key = { .mv_size = sizeof(node_id), .mv_data = &node_id };
    if (mdb_del(txn, heap_db, &key, NULL) == MDB_NOTFOUND)
        return MTL_ERROR_INVALID_ARGUMENT;

    return MTL_SUCCESS;
}

int mtl_heap_increase_key(MDB_txn *txn, mtl_heap_node_id node_id, uint64_t key, mtl_heap_node_id *updated_node_id) {

    mtl_ensure_heap_db_open(txn);

    int res;

    res = mtl_heap_delete(txn, node_id);
    if (res != MTL_SUCCESS) {
        return res;
    }

    return mtl_heap_insert(txn, key, 0, updated_node_id);
}

int mtl_reset_heap_db() {
    heap_db = 0;
    return MTL_SUCCESS;
}
