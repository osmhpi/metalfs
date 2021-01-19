#pragma once

#include "mtl_definitions.h"
#include "snap_action_metal.h"

namespace metal {
namespace fpga {

typedef struct {
    mtl_extent_offset_t current_extent;
    snapu64_t current_pblock;
    snapu64_t extents_begin[MaxExtentsPerFile];
    snapu64_t extents_count[MaxExtentsPerFile];
    snapu64_t extents_nextlblock[MaxExtentsPerFile];
} mtl_extmap_t;


void mtl_extmap_load(mtl_extmap_t & map,
                    snapu64_t extent_address,
                    snap_membus_512_t * mem);

mtl_bool_t mtl_extmap_seek(mtl_extmap_t & map, snapu64_t lblock);
void mtl_extmap_next(mtl_extmap_t & map);
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
