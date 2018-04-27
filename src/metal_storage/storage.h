#pragma once

#include <stdint.h>

typedef struct mtl_storage_metadata {
    uint64_t num_blocks;
    uint64_t block_size;
} mtl_storage_metadata;

typedef struct mtl_file_extent {
    uint64_t offset;
    uint64_t length;
} mtl_file_extent;

int mtl_storage_initialize();
int mtl_storage_deinitialize();

int mtl_storage_get_metadata(mtl_storage_metadata *metadata);

int mtl_storage_set_active_read_extent_list(const mtl_file_extent *extents, uint64_t length);
int mtl_storage_set_active_write_extent_list(const mtl_file_extent *extents, uint64_t length);

int mtl_storage_write(
    uint64_t offset, // The target file offset in bytes
    void *buffer,    // The source buffer
    uint64_t length  // Number of bytes
);

int mtl_storage_read(
    uint64_t offset, // The source file offset in bytes
    void *buffer,    // The target buffer
    uint64_t length  // Number of bytes
);
