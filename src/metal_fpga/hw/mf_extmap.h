#ifndef __MF_EXTMAP_H__
#define __MF_EXTMAP_H__

#include "mf_definitions.h"

typedef struct {
    mf_extent_count_t extent_count;
    mf_extent_offset_t current_extent;
    snapu64_t block_count;
    snapu64_t current_lblock;
    snapu64_t current_pblock;
    snapu64_t extents_begin[MF_EXTENT_COUNT];
    snapu64_t extents_count[MF_EXTENT_COUNT];
    snapu64_t extents_nextlblock[MF_EXTENT_COUNT];
} mf_extmap_t;


void mf_extmap_load(mf_extmap_t & map,
                    mf_extent_count_t extent_count,
                    snapu64_t extent_address,
                    snap_membus_t * mem);

mf_bool_t mf_extmap_seek(mf_extmap_t & map, snapu64_t lblock);
mf_bool_t mf_extmap_next(mf_extmap_t & map);
mf_bool_t mf_extmap_seek_extent(mf_extmap_t & map, mf_extent_offset_t extent);
mf_bool_t mf_extmap_next_extent(mf_extmap_t & map);

snapu64_t mf_extmap_block_count(mf_extmap_t & map);
mf_extent_count_t mf_extmap_extent_count(mf_extmap_t & map);
mf_bool_t mf_extmap_last_block(mf_extmap_t & map);
mf_bool_t mf_extmap_last_extent(mf_extmap_t & map);

snapu64_t mf_extmap_lblock(mf_extmap_t & map);
snapu64_t mf_extmap_pblock(mf_extmap_t & map);
mf_extent_offset_t mf_extmap_extent(mf_extmap_t & map);
snapu64_t mf_extmap_remaining_blocks(mf_extmap_t & map);
snapu64_t mf_extmap_remaining_blocks(mf_extmap_t & map, snapu64_t lblock);


#endif // __MF_EXTMAP_H__
