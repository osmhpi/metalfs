#include "mf_file.h"

#include "hls_globalmem.h"

#include "mf_definitions.h"
#include "mf_endian.h"


typedef struct
{
    snapu8_t flags;
    snapu16_t extent_count;
    snapu64_t block_count;
    snapu64_t current_pblock;
    snapu64_t current_lblock;
    snapu64_t block_buffer_address;
} mf_slot_state_t;
#define MF_SLOT_FLAG_OPEN 0x1
#define MF_SLOT_FLAG_ACTIVE 0x2


static snapu64_t mf_extents_begin[MF_SLOT_COUNT][MF_EXTENT_COUNT];
static snapu64_t mf_extents_count[MF_SLOT_COUNT][MF_EXTENT_COUNT];
static snapu64_t mf_extents_lastblock[MF_SLOT_COUNT][MF_EXTENT_COUNT];
static mf_slot_state_t mf_slots[MF_SLOT_COUNT];

mf_block_t mf_file_buffers[MF_SLOT_COUNT];


static void mf_extract_extents(mf_extent_t extents[MF_EXTENTS_PER_LINE], const snap_membus_t * mem, uint64_t address)
{
    snap_membus_t line = mem[MFB_ADDRESS(address)];
    //mf_extent_t extents[MF_EXTENTS_PER_LINE];
    for (mf_extent_count_t i_extent; i_extent < MF_EXTENTS_PER_LINE; ++i_extent)
    {
        extents[i_extent].block_begin = mf_get64(line, i_extent * sizeof(mf_extent_t));
        extents[i_extent].block_count = mf_get64(line, i_extent * sizeof(mf_extent_t) + sizeof(snapu64_t));
    }
}

static mf_bool_t mf_fill_extents_indirect(mf_slot_offset_t slot,
                                     mf_extent_count_t extent_count,
                                     snapu64_t address,
                                     snap_membus_t * mem)
{
    snapu64_t line_address = address;
    mf_extent_t line_extents[MF_EXTENTS_PER_LINE];
    snapu64_t last_block = 0;
    for (mf_extent_count_t i_extent = 0; i_extent < extent_count; ++i_extent)
    {
        mf_line_extent_offset_t o_line_extent = i_extent & MF_MASK(MF_EXTENTS_PER_LINE_W,0);
        if (o_line_extent == 0)
        {
            mf_extract_extents(line_extents, mem, MFB_ADDRESS(line_address));
            line_address += MFB_INCREMENT;
        }
        mf_extent_t & extent = line_extents[o_line_extent];
        mf_extents_begin[slot][i_extent] = extent.block_begin;
        mf_extents_count[slot][i_extent] = extent.block_count;
        last_block += extent.block_count;
        mf_extents_lastblock[slot][i_extent] = last_block;
    }
    mf_slots[slot].extent_count = extent_count;
    mf_slots[slot].block_count = last_block;
    return MF_TRUE;
}

static mf_bool_t mf_slot_open(mf_slot_offset_t slot, mf_extent_count_t extent_count)
{
    if (mf_slots[slot].flags & MF_SLOT_FLAG_OPEN)
    {
        return MF_FALSE;
    }
    mf_slots[slot].flags |= MF_SLOT_FLAG_OPEN;
    mf_slots[slot].flags &= ~MF_SLOT_FLAG_ACTIVE;
    mf_slots[slot].extent_count = 0;
    mf_slots[slot].block_count = 0;
    mf_slots[slot].current_pblock = 0;
    mf_slots[slot].current_lblock = 0;

    //use the first 64k of DRAM as block buffers with fixed slot mapping
    mf_slots[slot].block_buffer_address = ((uint64_t)slot) << 12;

    return MF_TRUE;
}


mf_bool_t mf_file_open(mf_slot_offset_t slot,
                       mf_extent_count_t extent_count,
                       snapu64_t buffer_address,
                       snap_membus_t * mem)
{
    return mf_slot_open(slot, extent_count) &&
            mf_fill_extents_indirect(slot, extent_count, buffer_address, mem);
}

mf_bool_t mf_file_close(mf_slot_offset_t slot)
{

    if (!(mf_slots[slot].flags & MF_SLOT_FLAG_OPEN))
    {
        return MF_FALSE;
    }
    if (mf_slots[slot].flags & MF_SLOT_FLAG_ACTIVE)
    {
        //TODO-lw flush block
        mf_slots[slot].flags &= ~MF_SLOT_FLAG_ACTIVE;
    }
    mf_slots[slot].flags &= ~MF_SLOT_FLAG_OPEN;
    mf_slots[slot].extent_count = 0;
    mf_slots[slot].current_pblock = 0;
    mf_slots[slot].current_lblock = 0;

    return MF_TRUE;
}


mf_bool_t mf_file_is_open(mf_slot_offset_t slot)
{
    return !!(mf_slots[slot].flags & MF_SLOT_FLAG_OPEN); 
}

mf_bool_t mf_file_is_active(mf_slot_offset_t slot)
{
    return !!(mf_slots[slot].flags & MF_SLOT_FLAG_ACTIVE);
}

mf_bool_t mf_file_at_end(mf_slot_offset_t slot)
{
    return mf_slots[slot].current_lblock == mf_slots[slot].block_count - 1;
}

snapu16_t mf_file_get_extent_count(mf_slot_offset_t slot)
{
    return mf_slots[slot].extent_count;

}

snapu64_t mf_file_get_block_count(mf_slot_offset_t slot)
{
    return mf_slots[slot].block_count;

}

snapu64_t mf_file_get_lblock(mf_slot_offset_t slot)
{
    //TODO-lw return INVALID_BLOCK_NUMBER if inactive
    return mf_slots[slot].current_lblock;
}

snapu64_t mf_file_get_pblock(mf_slot_offset_t slot)
{
    //TODO-lw return INVALID_BLOCK_NUMBER if inactive
    return mf_slots[slot].current_pblock;
}

snapu64_t mf_file_map_pblock(mf_slot_offset_t slot, snapu64_t logical_block_number)
{
    mf_extent_count_t extent_count = mf_slots[slot].extent_count;
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


mf_bool_t mf_file_seek(snap_membus_t * ddr, mf_slot_offset_t slot, snapu64_t lblock, mf_bool_t dirty)
{
    if (lblock < mf_slots[slot].block_count)
    {
        if (dirty)
        {
            mf_file_flush(ddr, slot);
        }
        mf_slots[slot].current_lblock = lblock;
        mf_slots[slot].current_pblock = mf_file_map_pblock(slot, lblock);
        //TODO-lw READ BLOCK current_pblock
        mf_slots[slot].flags |= MF_SLOT_FLAG_ACTIVE;
        return true;
    }
    return false;
}

mf_bool_t mf_file_next(snap_membus_t * ddr, mf_slot_offset_t slot, mf_bool_t dirty)
{
    if (!mf_file_at_end(slot))
    {
        //TODO-lw improve getting of consecutive pblocks (use current extent)
        if (dirty)
        {
            mf_file_flush(ddr, slot);
        }
        mf_slots[slot].current_lblock = mf_slots[slot].current_lblock + 1;
        mf_slots[slot].current_pblock = mf_file_map_pblock(slot, mf_slots[slot].current_lblock);
        //TODO-lw READ BLOCK current_pblock
        mf_slots[slot].flags |= MF_SLOT_FLAG_ACTIVE;
        return true;
    }
    return false;
}

mf_bool_t mf_file_flush(snap_membus_t * ddr, mf_slot_offset_t slot)
{
    if (mf_slots[slot].flags & MF_SLOT_FLAG_ACTIVE)
    {
        //TODO-lw WRITE BLOCK current_pblock
        mf_slots[slot].flags &= ~MF_SLOT_FLAG_ACTIVE;
    }
    return true;
}

//-----------------------------------------------------------------------------
//--- TESTBENCH ---------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef NO_SYNTH

int main()
{
    return 0;
}

#endif /* NO_SYNTH */
