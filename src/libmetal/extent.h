#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <lmdb.h>

typedef enum mtl_extent_status {
    MTL_FREE,
    MTL_RESERVED,
    MTL_COMMITTED
} mtl_extent_status;

typedef struct mtl_extent {
    uint64_t offset;
    uint64_t length;
    mtl_extent_status status;

    // Part of a hollow heap
    uint64_t h_second_parent;
    uint64_t h_next;
    uint64_t h_child;
    uint64_t h_rank;
    bool h_hollow;
} mtl_extent;

uint64_t mtl_reserve_extent(uint64_t size, uint64_t *offset);
int mtl_commit_extent(uint64_t offset, uint64_t len);
int mtl_free_extent(uint64_t offset);

int mtl_reset_extents_db();
