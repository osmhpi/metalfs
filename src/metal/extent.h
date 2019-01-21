#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <lmdb.h>

#include "../metal_storage/storage.h"

int mtl_initialize_extents(MDB_txn *txn, uint64_t blocks);

uint64_t mtl_reserve_extent(MDB_txn *txn, uint64_t size, mtl_file_extent *last_extent, uint64_t *offset, bool commit);
int mtl_truncate_extent(MDB_txn *txn, uint64_t offset, uint64_t len);
int mtl_free_extent(MDB_txn *txn, uint64_t offset);

int mtl_dump_extents(MDB_txn *txn);

int mtl_reset_extents_db();
