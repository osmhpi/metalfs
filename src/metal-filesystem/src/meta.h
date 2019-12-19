#pragma once

#include <stdint.h>

#include <lmdb.h>

uint64_t mtl_next_inode_id(MDB_txn *txn);
uint64_t mtl_next_heap_node_id(MDB_txn *txn);

int mtl_reset_meta_db();
