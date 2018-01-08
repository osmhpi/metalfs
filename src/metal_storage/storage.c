#include "storage.h"

#include <stddef.h>
#include <assert.h>

#ifdef WITH_SNAP

int mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
    return 0;
}

int mtl_storage_set_active_extent_list(const mtl_file_extent *extents, uint64_t length) {
    return 0;
}

int mtl_storage_write(uint64_t offset, void *buffer, uint64_t length) {
    return 0;
}

int mtl_storage_read(uint64_t offset, void *buffer, uint64_t length) {
    return 0;
}

#else

#define NUM_BLOCKS 128
#define BLOCK_SIZE 512

void *_storage = NULL;

mtl_file_extent *_extents = NULL;
uint64_t _extents_length;

int mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
    if (metadata) {
        metadata->num_blocks = NUM_BLOCKS;
        metadata->block_size = BLOCK_SIZE;
    }

    return 0;
}

int mtl_storage_set_active_extent_list(const mtl_file_extent *extents, uint64_t length) {
    free(_extents);

    _extents = malloc(length * sizeof(mtl_file_extent));
    memcpy(_extents, extents, length * sizeof(mtl_file_extent));
    _extents_length = length;
}

int mtl_storage_write(uint64_t offset, void *buffer, uint64_t length) {
    if (!_storage) {
        _storage = malloc(NUM_BLOCKS * BLOCK_SIZE);
    }

    assert(offset + length < NUM_BLOCKS * BLOCK_SIZE);

    uint64_t current_offset = offset;

    uint64_t current_extent = 0;
    uint64_t current_extent_offset = 0;
    while (length > 0) {
        // Determine the extent for the next byte to be written
        while (current_extent_offset + (_extents[current_extent].length * BLOCK_SIZE) <= current_extent_offset) {
            current_extent_offset += _extents[current_extent].length * BLOCK_SIZE;
            ++current_extent;

            // The caller has to make sure the file is large enough to write
            assert(current_extent < _extents_length);
        }

        uint64_t current_extent_write_pos = current_offset - current_extent_offset;
        uint64_t current_extent_write_length =
            (_extents[current_extent].length * BLOCK_SIZE) - current_extent_write_pos;
        if (current_extent_write_length > length)
            current_extent_write_length = length;

        memcpy(_storage + (_extents[current_extent].offset * BLOCK_SIZE) + current_extent_write_pos,
               buffer + (current_offset - offset),
               current_extent_write_length);

        current_offset += current_extent_write_length;
        length -= current_extent_write_length;
    }
}

int mtl_storage_read(uint64_t offset, void *buffer, uint64_t length) {
    if (!_storage) {
        _storage = malloc(NUM_BLOCKS * BLOCK_SIZE);
    }

    // TODO
}

#endif
