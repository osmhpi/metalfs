#pragma once

#include "mtl_definitions.h"

namespace metal {
namespace fpga {

typedef struct {
    mtl_extent_count_t extent_count;
    mtl_extent_offset_t current_extent;
    snapu64_t block_count;
    snapu64_t current_lblock;
    snapu64_t current_pblock;
    snapu64_t extents_begin[MTL_EXTENT_COUNT];
    snapu64_t extents_count[MTL_EXTENT_COUNT];
    snapu64_t extents_nextlblock[MTL_EXTENT_COUNT];
} mtl_extmap_t;


void mtl_extmap_load(mtl_extmap_t & map,
                    mtl_extent_count_t extent_count,
                    snapu64_t extent_address,
                    snap_membus_t * mem);

mtl_bool_t mtl_extmap_seek(mtl_extmap_t & map, snapu64_t lblock);
mtl_bool_t mtl_extmap_next(mtl_extmap_t & map);
mtl_bool_t mtl_extmap_seek_extent(mtl_extmap_t & map, mtl_extent_offset_t extent);
mtl_bool_t mtl_extmap_next_extent(mtl_extmap_t & map);

snapu64_t mtl_extmap_block_count(mtl_extmap_t & map);
mtl_extent_count_t mtl_extmap_extent_count(mtl_extmap_t & map);
mtl_bool_t mtl_extmap_last_block(mtl_extmap_t & map);
mtl_bool_t mtl_extmap_last_extent(mtl_extmap_t & map);

snapu64_t mtl_extmap_lblock(mtl_extmap_t & map);
snapu64_t mtl_extmap_pblock(mtl_extmap_t & map);
mtl_extent_offset_t mtl_extmap_extent(mtl_extmap_t & map);
snapu64_t mtl_extmap_remaining_blocks(mtl_extmap_t & map);
snapu64_t mtl_extmap_remaining_blocks(mtl_extmap_t & map, snapu64_t lblock);

}  // namespace fpga
}  // namespace metal
