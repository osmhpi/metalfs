#pragma once

#include <stdint.h>

#include <lmdb.h>

int mtl_heap_extract_max(MDB_txn *txn, uint64_t *max_value);
