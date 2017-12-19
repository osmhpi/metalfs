#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <lmdb.h>

int mtl_heap_insert(MDB_txn *txn, uint64_t key, uint64_t value);
int mtl_heap_extract_max(MDB_txn *txn, uint64_t *max_value);
// Because of a missing external unique identifier, we currently don't
// offer a decrease_key operation.

int mtl_reset_heap_db();

#ifdef __cplusplus
}
#endif
