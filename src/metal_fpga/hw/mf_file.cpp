#include "mf_file.h"

#include "hls_globalmem.h"

#include "mf_definitions.h"
#include "mf_endian.h"


mf_bool_t mf_file_open(mf_fileslot_t & slot,
                       snapu64_t block_buffer_address,
                       mf_extent_count_t extent_count,
                       snapu64_t extent_address,
                       snap_membus_t * mem)
{
    if (slot.is_open) {
        return MF_FALSE;
    }

    snapu64_t line_address = extent_address;
    snap_membus_t line = 0;
    snapu64_t last_block = 0;
    for (mf_extent_count_t i_extent = 0; i_extent < extent_count; ++i_extent) {
        mf_line_extent_offset_t o_line_extent = i_extent & MF_MASK(MF_EXTENTS_PER_LINE_W,0);
        if (o_line_extent == 0) {
            line = mem[MFB_ADDRESS(address)];
            line_address += MFB_INCREMENT;
        }
        snapu64_t begin = mf_get64(line, o_line_extent * 16);
        snapu64_t count = mf_get64(line, o_line_extent * 16 + 8);
        last_block += count;
        mf_extents_begin[slot][i_extent] = begin;
        mf_extents_count[slot][i_extent] = count;
        mf_extents_lastblock[slot][i_extent] = last_block;
    }

    slot.is_open = true;
    slot.is_active = false;
    slot.current_pblock = 0;
    slot.current_lblock = 0;
    slot.block_buffer_address = block_buffer_address;
    slot.extent_count = extent_count;
    slot.block_count = last_block;

    return MF_TRUE;
}

mf_bool_t mf_file_close(mf_fileslot_t & slot)
{
    if (!slot.is_open) {
        return MF_FALSE;
    }
    if (slot.is_active) {
        //TODO-lw flush block
        slot.is_active = false;
    }
    slot.is_open = false;
    slot.extent_count = 0;
    slot.current_pblock = 0;
    slot.current_lblock = 0;

    return MF_TRUE;
}


mf_bool_t mf_file_is_open(mf_fileslot_t & slot)
{
    return slot.is_open;
}

mf_bool_t mf_file_is_active(mf_fileslot_t & slot)
{
    return slot.is_active;
}

mf_bool_t mf_file_at_end(mf_fileslot_t & slot)
{
    return slot.current_lblock == slot.block_count - 1;
}

snapu16_t mf_file_get_extent_count(mf_fileslot_t & slot)
{
    return slot.extent_count;

}

snapu64_t mf_file_get_block_count(mf_fileslot_t & slot)
{
    return slot.block_count;

}

snapu64_t mf_file_get_lblock(mf_fileslot_t & slot)
{
    //TODO-lw return INVALID_BLOCK_NUMBER if inactive
    return slot.current_lblock;
}

snapu64_t mf_file_get_pblock(mf_fileslot_t & slot)
{
    //TODO-lw return INVALID_BLOCK_NUMBER if inactive
    return slot.current_pblock;
}

snapu64_t mf_file_map_pblock(mf_fileslot_t & slot, snapu64_t logical_block_number)
{
    mf_extent_count_t extent_count = slot.extent_count;
    uint64_t extent_begin = 0;
    uint64_t next_extent_begin = 0;
    for (mf_extent_count_t i_extent = 0; i_extent < extent_count; ++i_extent)
    {
#pragma HLS UNROLL factor=16
        next_extent_begin = mf_extents_lastblock[slot][i_extent];
        if (extent_begin <= logical_block_number &&
               logical_block_number < next_extent_begin)
        {
            snapu64_t offset = logical_block_number - extent_begin;
            snapu64_t base = mf_extents_begin[slot][i_extent];
            return base + offset;
        }
        extent_begin = next_extent_begin;
    }
    return 0; //TODO-lw define INVALID_BLOCK_NUMBER
}


mf_bool_t mf_file_seek(mf_fileslot_t & slot, mf_filebuf_t & buffer, snap_membus_t * ddr, snapu64_t lblock, mf_bool_t dirty)
{
    if (lblock < slot.block_count)
    {
        if (dirty)
        {
            mf_file_flush(ddr, slot);
        }
        slot.current_lblock = lblock;
        slot.current_pblock = mf_file_map_pblock(slot, lblock);
        //TODO-lw READ BLOCK current_pblock
        slot.is_active = true;
        return true;
    }
    return false;
}

mf_bool_t mf_file_next(mf_fileslot_t & slot, mf_filebuf_t & buffer, snap_membus_t * ddr, mf_bool_t dirty)
{
    if (!mf_file_at_end(slot))
    {
        //TODO-lw improve getting of consecutive pblocks (use current extent)
        if (dirty)
        {
            mf_file_flush(ddr, slot);
        }
        slot.current_lblock = slot.current_lblock + 1;
        slot.current_pblock = mf_file_map_pblock(slot, slot.current_lblock);
        //TODO-lw READ BLOCK current_pblock
        slot.is_active = true;
        return true;
    }
    return false;
}

mf_bool_t mf_file_flush(mf_fileslot_t & slot, mf_filebuf_t & buffer, snap_membus_t * ddr)
{
    if (slot.is_active)
    {
        //TODO-lw WRITE BLOCK current_pblock
        slot.is_active = false;
    }
    return true;
}

