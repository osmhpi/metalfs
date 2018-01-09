#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <lmdb.h>

int mtl_initialize_extents(MDB_txn *txn, uint64_t blocks);

uint64_t mtl_reserve_extent(MDB_txn *txn, uint64_t size, uint64_t *offset, bool commit);
int mtl_truncate_extent(MDB_txn *txn, uint64_t offset, uint64_t len);
int mtl_free_extent(MDB_txn *txn, uint64_t offset);

int mtl_reset_extents_db();

#ifdef __cplusplus
}
#endif
