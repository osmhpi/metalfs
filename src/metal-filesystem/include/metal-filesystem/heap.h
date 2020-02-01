#pragma once

#include <stdint.h>

#include <lmdb.h>

typedef struct mtl_heap_node_id { uint64_t k; uint64_t v; } mtl_heap_node_id;
extern mtl_heap_node_id INVALID_NODE;

int mtl_heap_insert(MDB_txn *txn, uint64_t key, uint64_t value, mtl_heap_node_id *node_id);
int mtl_heap_extract_max(MDB_txn *txn, uint64_t *max_value);
int mtl_heap_delete(MDB_txn *txn, mtl_heap_node_id node_id);
int mtl_heap_increase_key(MDB_txn *txn, mtl_heap_node_id node_id, uint64_t key, mtl_heap_node_id *updated_node_id);

int mtl_reset_heap_db();
