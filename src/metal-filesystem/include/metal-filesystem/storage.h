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

typedef struct mtl_context mtl_context;

typedef struct mtl_storage_backend {
  int (*initialize)(void *storage_context);
  int (*deinitialize)(void *storage_context);

  int (*get_metadata)(void *storage_context, mtl_storage_metadata *metadata);

  int (*write)(mtl_context *context, void *storage_context, uint64_t inode_id,
               uint64_t offset,     // The target file offset in bytes
               const void *buffer,  // The source buffer
               uint64_t length      // Number of bytes
  );

  int (*read)(mtl_context *context, void *storage_context, uint64_t inode_id,
              uint64_t offset,  // The source file offset in bytes
              void *buffer,     // The target buffer
              uint64_t length   // Number of bytes
  );

  void *context;

} mtl_storage_backend;

extern mtl_storage_backend in_memory_storage;
