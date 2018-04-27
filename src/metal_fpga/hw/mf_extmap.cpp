#include "mf_extmap.h"

#include "mf_endian.h"

static snapu64_t mf_extmap_firstlblock(mf_extmap_t & map, mf_extent_offset_t extent) {
    if (extent == 0) {
        return 0;
    } else {
        return map.extents_nextlblock[extent-1];
    }
}

void mf_extmap_load(mf_extmap_t & map,
                    mf_extent_count_t extent_count,
                    snapu64_t extent_address,
                    snap_membus_t * mem)
{
    snapu64_t line_address = extent_address;
    snap_membus_t line = 0;
    snapu64_t last_block = 0;
    for (mf_extent_count_t i_extent = 0; i_extent < extent_count; ++i_extent) {
        mf_line_extent_offset_t o_line_extent = i_extent & MF_MASK(MF_EXTENTS_PER_LINE_W,0);
        if (o_line_extent == 0) {
            line = mem[MFB_ADDRESS(line_address)];
            line_address += MFB_INCREMENT;
        }
        snapu64_t begin = mf_get64(line, o_line_extent * 16);
        snapu64_t count = mf_get64(line, o_line_extent * 16 + 8);
        last_block += count;
        map.extents_begin[i_extent] = begin;
        map.extents_count[i_extent] = count;
        map.extents_nextlblock[i_extent] = last_block;
    }

    map.extent_count = extent_count;
    map.current_extent = 0;
    map.block_count = last_block;
    map.current_lblock = 0;
    map.current_pblock = map.extents_begin[0];
}

mf_bool_t mf_extmap_seek(mf_extmap_t & map, snapu64_t lblock) {
    if (lblock >= map.block_count) {
        return MF_FALSE;
    }

    map.current_lblock = lblock;
    for (mf_extent_count_t i_extent = 0; i_extent < map.extent_count; ++i_extent) {
#pragma HLS UNROLL factor=16
        uint64_t extent_begin = mf_extmap_firstlblock(map, i_extent);
        uint64_t extent_end = map.extents_nextlblock[i_extent];
        if (extent_begin <= lblock && lblock < extent_end) {
            snapu64_t offset = lblock - extent_begin;
            snapu64_t base = map.extents_begin[i_extent];
            map.current_pblock = base + offset;
            map.current_extent = i_extent;
        }
    }

    return MF_TRUE;
}

mf_bool_t mf_extmap_next(mf_extmap_t & map) {
    if (mf_extmap_last_block(map)) {
        return MF_FALSE;
    }

    map.current_lblock = map.current_lblock + 1;
    snapu64_t boffset = map.current_lblock - mf_extmap_firstlblock(map, map.current_extent);
    if (boffset < map.extents_count[map.current_extent]) {
        map.current_pblock = map.current_pblock + 1;
    } else {
        map.current_extent = map.current_extent + 1;
        map.current_pblock = map.extents_begin[map.current_extent];
    }

    return MF_TRUE;
}

mf_bool_t mf_extmap_seek_extent(mf_extmap_t & map, mf_extent_offset_t extent) {
    if (extent >= map.extent_count) {
        return MF_FALSE;
    }

    map.current_extent = extent;
    map.current_lblock = mf_extmap_firstlblock(map, map.current_extent);
    map.current_pblock = map.extents_begin[map.current_extent];

    return MF_TRUE;
}

mf_bool_t mf_extmap_next_extent(mf_extmap_t & map) {
    if (mf_extmap_last_extent(map)) {
        return MF_FALSE;
    }

    map.current_extent = map.current_extent + 1;
    map.current_lblock = mf_extmap_firstlblock(map, map.current_extent);
    map.current_pblock = map.extents_begin[map.current_extent];

    return MF_TRUE;
}

snapu64_t mf_extmap_block_count(mf_extmap_t & map) {
    return map.block_count;
}

mf_extent_count_t mf_extmap_extent_count(mf_extmap_t & map) {
    return map.extent_count;
}

mf_bool_t mf_extmap_last_block(mf_extmap_t & map) {
    return map.current_lblock >= map.block_count - 1;
}

mf_bool_t mf_extmap_last_extent(mf_extmap_t & map) {
    return map.current_extent >= map.extent_count - 1;
}

snapu64_t mf_extmap_lblock(mf_extmap_t & map) {
    return map.current_lblock;
}

snapu64_t mf_extmap_pblock(mf_extmap_t & map) {
    return map.current_pblock;
}

mf_extent_offset_t mf_extmap_extent(mf_extmap_t & map) {
    return map.current_extent;
}

snapu64_t mf_extmap_remaining_blocks(mf_extmap_t & map) {
    snapu64_t bcount = map.extents_count[map.current_extent];
    snapu64_t boffset = map.current_lblock - mf_extmap_firstlblock(map, map.current_extent);

    return bcount - boffset;
}
snapu64_t mf_extmap_remaining_blocks(mf_extmap_t & map, snapu64_t lblock) {
    if (lblock <= map.current_lblock) {
        return 0;
    }
    snapu64_t bcount = map.extents_count[map.current_extent];
    snapu64_t boffset = map.current_lblock - mf_extmap_firstlblock(map, map.current_extent);
    snapu64_t btoext = bcount - boffset;
    snapu64_t btoblk = lblock - map.current_lblock;

    if (btoext < btoblk) {
        return btoext;
    } else {
        return btoblk;
    }
}

// snapu64_t mf_file_map_pblock(mf_fileslot_t & slot, snapu64_t logical_block_number)
// {
//     mf_extent_count_t extent_count = slot.extent_count;
//     uint64_t extent_begin = 0;
//     uint64_t next_extent_begin = 0;
//     for (mf_extent_count_t i_extent = 0; i_extent < extent_count; ++i_extent)
//     {
// #pragma HLS UNROLL factor=16
//         next_extent_begin = mf_extents_lastblock[slot][i_extent];
//         if (extent_begin <= logical_block_number &&
//                logical_block_number < next_extent_begin)
//         {
//             snapu64_t offset = logical_block_number - extent_begin;
//             snapu64_t base = mf_extents_begin[slot][i_extent];
//             return base + offset;
//         }
//         extent_begin = next_extent_begin;
//     }
//     return 0; //TODO-lw define INVALID_BLOCK_NUMBER
// }


// mf_bool_t mf_file_seek(mf_fileslot_t & slot, mf_filebuf_t & buffer, snap_membus_t * ddr, snapu64_t lblock, mf_bool_t dirty)
// {
//     if (lblock < slot.block_count)
//     {
//         if (dirty)
//         {
//             mf_file_flush(ddr, slot);
//         }
//         slot.current_lblock = lblock;
//         slot.current_pblock = mf_file_map_pblock(slot, lblock);
//         //TODO-lw READ BLOCK current_pblock
//         slot.is_active = true;
//         return true;
//     }
//     return false;
// }

// mf_bool_t mf_file_next(mf_fileslot_t & slot, mf_filebuf_t & buffer, snap_membus_t * ddr, mf_bool_t dirty)
// {
//     if (!mf_file_at_end(slot))
//     {
//         //TODO-lw improve getting of consecutive pblocks (use current extent)
//         if (dirty)
//         {
//             mf_file_flush(ddr, slot);
//         }
//         slot.current_lblock = slot.current_lblock + 1;
//         slot.current_pblock = mf_file_map_pblock(slot, slot.current_lblock);
//         //TODO-lw READ BLOCK current_pblock
//         slot.is_active = true;
//         return true;
//     }
//     return false;
// }

// mf_bool_t mf_file_flush(mf_fileslot_t & slot, mf_filebuf_t & buffer, snap_membus_t * ddr)
// {
//     if (slot.is_active)
//     {
//         //TODO-lw WRITE BLOCK current_pblock
//         slot.is_active = false;
//     }
//     return true;
// }

