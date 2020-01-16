#pragma once

typedef struct mtl_storage_metadata {
  uint64_t num_blocks;
  uint64_t block_size;
} mtl_storage_metadata;

typedef struct mtl_file_extent {
  uint64_t offset;
  uint64_t length;
} mtl_file_extent;

typedef struct mtl_storage_backend {
  int (*initialize)();
  int (*deinitialize)();

  int (*get_metadata)(mtl_storage_metadata *metadata);

  int (*set_active_read_extent_list)(const mtl_file_extent *extents,
                                     uint64_t length);
  int (*set_active_write_extent_list)(const mtl_file_extent *extents,
                                      uint64_t length);

  int (*write)(uint64_t offset,     // The target file offset in bytes
               const void *buffer,  // The source buffer
               uint64_t length      // Number of bytes
  );

  int (*read)(uint64_t offset,  // The source file offset in bytes
              void *buffer,     // The target buffer
              uint64_t length   // Number of bytes
  );

} mtl_storage_backend;

extern mtl_storage_backend in_memory_storage;
