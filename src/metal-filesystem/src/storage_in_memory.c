#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <metal-filesystem/metal.h>

// Reference implementation of a storage backend, backed by an in-memory buffer

// 128 MiB
#define NUM_BLOCKS 128 * 256
#define BLOCK_SIZE 4096

void *_storage = NULL;

mtl_file_extent *_extents = NULL;
uint64_t _extents_length;

int mtl_storage_initialize(void *storage_context) {
  _storage = malloc(NUM_BLOCKS * BLOCK_SIZE);
  return MTL_SUCCESS;
}

int mtl_storage_deinitialize(void *storage_context) {
  if (_storage) {
    free(_storage);
    _storage = NULL;
  }
  return MTL_SUCCESS;
}

int mtl_storage_get_metadata(void *storage_context,
                             mtl_storage_metadata *metadata) {
  if (metadata) {
    metadata->num_blocks = NUM_BLOCKS;
    metadata->block_size = BLOCK_SIZE;
  }

  return MTL_SUCCESS;
}

int mtl_storage_set_active_write_extent_list(void *storage_context,
                                             const mtl_file_extent *extents,
                                             uint64_t length) {
  free(_extents);

  _extents = malloc(length * sizeof(mtl_file_extent));
  memcpy(_extents, extents, length * sizeof(mtl_file_extent));
  _extents_length = length;

  return MTL_SUCCESS;
}

int mtl_storage_set_active_read_extent_list(void *storage_context,
                                            const mtl_file_extent *extents,
                                            uint64_t length) {
  free(_extents);

  _extents = malloc(length * sizeof(mtl_file_extent));
  memcpy(_extents, extents, length * sizeof(mtl_file_extent));
  _extents_length = length;

  return MTL_SUCCESS;
}

int mtl_storage_write(void *storage_context, uint64_t offset,
                      const void *buffer, uint64_t length) {
  if (!_storage) {
    _storage = malloc(NUM_BLOCKS * BLOCK_SIZE);
  }

  assert(offset + length < NUM_BLOCKS * BLOCK_SIZE);

  uint64_t current_offset = offset;

  uint64_t current_extent = 0;
  uint64_t current_extent_offset = 0;
  while (length > 0) {
    // Determine the extent for the next byte to be written
    while (current_extent_offset +
               (_extents[current_extent].length * BLOCK_SIZE) <=
           current_offset) {
      current_extent_offset += _extents[current_extent].length * BLOCK_SIZE;
      ++current_extent;

      // The caller has to make sure the file is large enough to write
      assert(current_extent < _extents_length);
    }

    uint64_t current_extent_write_pos = current_offset - current_extent_offset;
    uint64_t current_extent_write_length =
        (_extents[current_extent].length * BLOCK_SIZE) -
        current_extent_write_pos;
    if (current_extent_write_length > length)
      current_extent_write_length = length;

    memcpy(_storage + (_extents[current_extent].offset * BLOCK_SIZE) +
               current_extent_write_pos,
           buffer + (current_offset - offset), current_extent_write_length);

    current_offset += current_extent_write_length;
    length -= current_extent_write_length;
  }

  return MTL_SUCCESS;
}

int mtl_storage_read(void *storage_context, uint64_t offset, void *buffer,
                     uint64_t length) {
  if (!_storage) {
    _storage = malloc(NUM_BLOCKS * BLOCK_SIZE);
  }

  assert(offset + length < NUM_BLOCKS * BLOCK_SIZE);

  uint64_t current_offset = offset;

  uint64_t current_extent = 0;
  uint64_t current_extent_offset = 0;
  while (length > 0) {
    // Determine the extent for the next byte to be read
    while (current_extent_offset +
               (_extents[current_extent].length * BLOCK_SIZE) <=
           current_offset) {
      current_extent_offset += _extents[current_extent].length * BLOCK_SIZE;
      ++current_extent;

      // The caller has to make sure the file is large enough to read
      assert(current_extent < _extents_length);
    }

    uint64_t current_extent_read_pos = current_offset - current_extent_offset;
    uint64_t current_extent_read_length =
        (_extents[current_extent].length * BLOCK_SIZE) -
        current_extent_read_pos;
    if (current_extent_read_length > length)
      current_extent_read_length = length;

    memcpy(buffer + (current_offset - offset),
           _storage + (_extents[current_extent].offset * BLOCK_SIZE) +
               current_extent_read_pos,
           current_extent_read_length);

    current_offset += current_extent_read_length;
    length -= current_extent_read_length;
  }

  return MTL_SUCCESS;
}

mtl_storage_backend in_memory_storage = {
    &mtl_storage_initialize,
    &mtl_storage_deinitialize,

    &mtl_storage_get_metadata,

    &mtl_storage_set_active_read_extent_list,
    &mtl_storage_set_active_write_extent_list,

    &mtl_storage_write,
    &mtl_storage_read,
    NULL};
