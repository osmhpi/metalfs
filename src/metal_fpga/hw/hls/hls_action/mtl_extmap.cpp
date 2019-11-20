#include "mtl_extmap.h"

#include "mtl_endian.h"

namespace metal {
namespace fpga {

static snapu64_t mtl_extmap_firstlblock(mtl_extmap_t & map, mtl_extent_offset_t extent) {
    if (extent == 0) {
        return 0;
    } else {
        return map.extents_nextlblock[extent-1];
    }
}

void mtl_extmap_load(mtl_extmap_t & map,
                    mtl_extent_count_t extent_count,
                    snapu64_t extent_address,
                    snap_membus_t * mem)
{
    snapu64_t line_address = extent_address;
    snap_membus_t line = 0;
    snapu64_t last_block = 0;
    for (mtl_extent_count_t i_extent = 0; i_extent < extent_count; ++i_extent) {
        mtl_line_extent_offset_t o_line_extent = i_extent & MTL_MASK(MTL_EXTENTS_PER_LINE_W, 0);
        if (o_line_extent == 0) {
            line = mem[MFB_ADDRESS(line_address)];
            line_address += MFB_INCREMENT;
        }
        snapu64_t begin = mtl_get64<0>(line);
        snapu64_t count = mtl_get64<8>(line);
        last_block += count;
        map.extents_begin[i_extent] = begin;
        map.extents_count[i_extent] = count;
        map.extents_nextlblock[i_extent] = last_block;
        line >>= (sizeof(snapu64_t) + sizeof(snapu64_t)) * 8;
    }

    map.extent_count = extent_count;
    map.current_extent = 0;
    map.block_count = last_block;
    map.current_lblock = 0;
    map.current_pblock = map.extents_begin[0];
}

mtl_bool_t mtl_extmap_seek(mtl_extmap_t & map, snapu64_t lblock) {
    if (lblock >= map.block_count) {
        return MTL_FALSE;
    }

    map.current_lblock = lblock;
    for (mtl_extent_count_t i_extent = 0; i_extent < map.extent_count; ++i_extent) {
// #pragma HLS UNROLL factor=16
        uint64_t extent_begin = mtl_extmap_firstlblock(map, i_extent);
        uint64_t extent_end = map.extents_nextlblock[i_extent];
        if (extent_begin <= lblock && lblock < extent_end) {
            snapu64_t offset = lblock - extent_begin;
            snapu64_t base = map.extents_begin[i_extent];
            map.current_pblock = base + offset;
            map.current_extent = i_extent;
        }
    }

    return MTL_TRUE;
}

mtl_bool_t mtl_extmap_next(mtl_extmap_t & map) {
    if (mtl_extmap_last_block(map)) {
        return MTL_FALSE;
    }

    map.current_lblock = map.current_lblock + 1;
    snapu64_t boffset = map.current_lblock - mtl_extmap_firstlblock(map, map.current_extent);
    if (boffset < map.extents_count[map.current_extent]) {
        map.current_pblock = map.current_pblock + 1;
    } else {
        map.current_extent = map.current_extent + 1;
        map.current_pblock = map.extents_begin[map.current_extent];
    }

    return MTL_TRUE;
}

mtl_bool_t mtl_extmap_seek_extent(mtl_extmap_t & map, mtl_extent_offset_t extent) {
    if (extent >= map.extent_count) {
        return MTL_FALSE;
    }

    map.current_extent = extent;
    map.current_lblock = mtl_extmap_firstlblock(map, map.current_extent);
    map.current_pblock = map.extents_begin[map.current_extent];

    return MTL_TRUE;
}

mtl_bool_t mtl_extmap_next_extent(mtl_extmap_t & map) {
    if (mtl_extmap_last_extent(map)) {
        return MTL_FALSE;
    }

    map.current_extent = map.current_extent + 1;
    map.current_lblock = mtl_extmap_firstlblock(map, map.current_extent);
    map.current_pblock = map.extents_begin[map.current_extent];

    return MTL_TRUE;
}

snapu64_t mtl_extmap_block_count(mtl_extmap_t & map) {
    return map.block_count;
}

mtl_extent_count_t mtl_extmap_extent_count(mtl_extmap_t & map) {
    return map.extent_count;
}

mtl_bool_t mtl_extmap_last_block(mtl_extmap_t & map) {
    return map.current_lblock >= map.block_count - 1;
}

mtl_bool_t mtl_extmap_last_extent(mtl_extmap_t & map) {
    return map.current_extent >= map.extent_count - 1;
}

snapu64_t mtl_extmap_lblock(mtl_extmap_t & map) {
    return map.current_lblock;
}

snapu64_t mtl_extmap_pblock(mtl_extmap_t & map) {
    return map.current_pblock;
}

mtl_extent_offset_t mtl_extmap_extent(mtl_extmap_t & map) {
    return map.current_extent;
}

snapu64_t mtl_extmap_remaining_blocks(mtl_extmap_t & map) {
    snapu64_t bcount = map.extents_count[map.current_extent];
    snapu64_t boffset = map.current_lblock - mtl_extmap_firstlblock(map, map.current_extent);

    return bcount - boffset;
}
snapu64_t mtl_extmap_remaining_blocks(mtl_extmap_t & map, snapu64_t lblock) {
    if (lblock <= map.current_lblock) {
        return 0;
    }
    snapu64_t bcount = map.extents_count[map.current_extent];
    snapu64_t boffset = map.current_lblock - mtl_extmap_firstlblock(map, map.current_extent);
    snapu64_t btoext = bcount - boffset;
    snapu64_t btoblk = lblock - map.current_lblock;

    if (btoext < btoblk) {
        return btoext;
    } else {
        return btoblk;
    }
}

}  // namespace fpga
}  // namespace metal
