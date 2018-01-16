#include <string.h>
#include <iostream>

#include "action_metalfpga.H"

#include "endianconv.hpp"

#define HW_RELEASE_LEVEL       0x00000013

using namespace std;

static snapu64_t mf_extents_begin[MF_SLOT_COUNT][MF_EXTENT_COUNT];
static snapu64_t mf_extents_count[MF_SLOT_COUNT][MF_EXTENT_COUNT];
static snapu64_t mf_extents_lastblock[MF_SLOT_COUNT][MF_EXTENT_COUNT];
static mf_slot_state_t mf_slots[MF_SLOT_COUNT];

// NVMe interface functions:

/**
 * This function represents a read command to the NVMe Controller and
 * is meant to return only if the command is completed.
 */
static void mf_read_block_dummy(uint64_t ram_block_address, uint64_t nvme_block_address)
{
    
}

/**
 * This function represents a read command to the NVMe Controller and
 * is meant to return only if the command is completed.
 */
static void mf_write_block_dummy(uint64_t ram_block_address, uint64_t nvme_block_address)
{
    
}

// Filemap functions:
static void mf_extract_extents(mf_extent_t extents[MF_EXTENTS_PER_LINE], const snap_membus_t * din_gmem, uint64_t address)
{
    snap_membus_t line = din_gmem[MFB_ADDRESS(address)];
    //mf_extent_t extents[MF_EXTENTS_PER_LINE];
    for (mf_extent_count_t i_extent; i_extent < MF_EXTENTS_PER_LINE; ++i_extent)
    {
        extents[i_extent].block_begin = mf_get64(line, i_extent * sizeof(mf_extent_t));
        extents[i_extent].block_count = mf_get64(line, i_extent * sizeof(mf_extent_t) + sizeof(snapu64_t));
    }
}

static mf_bool_t mf_fill_extents_mem(mf_slot_offset_t slot,
                                     mf_extent_count_t extent_count,
                                     snap_membus_t * din_gmem,
                                     snapu64_t address)
{
    snapu64_t line_address = address;
    mf_extent_t line_extents[MF_EXTENTS_PER_LINE];
    snapu64_t last_block = 0;
    for (mf_extent_count_t i_extent = 0; i_extent < extent_count; ++i_extent)
    {
        mf_line_extent_offset_t o_line_extent = i_extent & MF_MASK(MF_EXTENTS_PER_LINE_W,0);
        if (o_line_extent == 0)
        {
            mf_extract_extents(line_extents, din_gmem, MFB_ADDRESS(line_address));
            line_address += MFB_INCREMENT;
        }
        mf_extent_t & extent = line_extents[o_line_extent];
        mf_extents_begin[slot][i_extent] = extent.block_begin;
        mf_extents_count[slot][i_extent] = extent.block_count;
        last_block += extent.block_count;
        mf_extents_lastblock[slot][i_extent] = last_block;
    }
    return MF_TRUE;
}

static mf_bool_t mf_fill_extents_direct(mf_slot_offset_t slot,
                                        mf_extent_count_t extent_count,
                                        const mf_extent_t direct_extents[MF_EXTENT_DIRECT_COUNT])
{
    mf_extent_count_t i_extent = 0;
    snapu64_t last_block = 0;
    for (mf_extent_count_t i_extent = 0; i_extent < MF_EXTENT_DIRECT_COUNT && i_extent < extent_count; ++i_extent)
    {
        mf_extents_begin[slot][i_extent] = direct_extents[i_extent].block_begin;
        mf_extents_count[slot][i_extent] = direct_extents[i_extent].block_count;
        last_block += direct_extents[i_extent].block_count;
        mf_extents_lastblock[slot][i_extent] = last_block;
    }
    return MF_TRUE;
}

static mf_bool_t mf_slot_open(mf_slot_offset_t slot, mf_extent_count_t extent_count)
{
    if (mf_slots[slot].flags & MF_SLOT_FLAG_OPEN)
    {
        return MF_FALSE;
    }
    mf_slots[slot].flags |= MF_SLOT_FLAG_OPEN;
    mf_slots[slot].flags &= ~MF_SLOT_FLAG_BLOCK_ACTIVE;
    mf_slots[slot].extent_count = extent_count;
    mf_slots[slot].physical_block_number = 0;
    mf_slots[slot].logical_block_number = 0;

    //use the first 64k of DRAM as block buffers with fixed slot mapping
    mf_slots[slot].block_buffer_address = ((uint64_t)slot) << 12;

    return MF_TRUE;
}

static mf_bool_t mf_slot_close(mf_slot_offset_t slot)
{

    if (!(mf_slots[slot].flags & MF_SLOT_FLAG_OPEN))
    {
        return MF_FALSE;
    }
    if (mf_slots[slot].flags & MF_SLOT_FLAG_BLOCK_ACTIVE)
    {
        //TODO-lw flush block
        mf_slots[slot].flags &= ~MF_SLOT_FLAG_BLOCK_ACTIVE;
    }
    mf_slots[slot].flags &= ~MF_SLOT_FLAG_OPEN;
    mf_slots[slot].extent_count = 0;
    mf_slots[slot].physical_block_number = 0;
    mf_slots[slot].logical_block_number = 0;

    return MF_TRUE;
}

static snapu64_t mf_physical_block_number(mf_slot_offset_t slot, snapu64_t logical_block_number)
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

// ------------------------------------------------
// --------------- ACTION FUNCTIONS ---------------
// ------------------------------------------------

static snapu32_t action_filemap(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                mf_func_filemap_job_t job)
{
    if (job.slot >= MF_SLOT_COUNT)
    {
        return SNAP_RETC_FAILURE;
    }
    mf_slot_offset_t slot = job.slot;

    if ((job.flags & MF_FILEMAP_MODE) == MF_FILEMAP_MODE_UNMAP)
    {
        if (!mf_slot_close(slot))
        {
            return SNAP_RETC_FAILURE;
        }
    }
    else if ((job.flags & MF_FILEMAP_MODE) == MF_FILEMAP_MODE_MAP)
    {
        if (job.extent_count > MF_EXTENT_COUNT)
        {
            return SNAP_RETC_FAILURE;
        }
        mf_extent_count_t extent_count = job.extent_count;

        if (!mf_slot_open(slot, extent_count))
        {
            return SNAP_RETC_FAILURE;
        }
        if (job.flags & MF_FILEMAP_INDIRECT)
        {
            if (!mf_fill_extents_direct(slot, extent_count, job.extents.direct))
            {
                mf_slot_close(slot);
                return SNAP_RETC_FAILURE;
            }
        }
        else
        {
            if (!mf_fill_extents_mem(slot, extent_count, din_gmem, job.extents.indirect_address))
            {
                mf_slot_close(slot);
                return SNAP_RETC_FAILURE;
            }
        }
    }
    else if ((job.flags & MF_FILEMAP_MODE) == MF_FILEMAP_MODE_TEST)
    {
        snap_membus_t line = din_gmem[job.extents.indirect_address>>ADDR_RIGHT_SHIFT];
        snapu64_t logical_block_number = mf_get64(line, 0);
        snapu64_t physical_block_number = mf_physical_block_number(slot, logical_block_number);
        mf_set64(line, 0, physical_block_number);
        dout_gmem[job.extents.indirect_address>>ADDR_RIGHT_SHIFT] = line;
    }
    else
    {
        return SNAP_RETC_FAILURE;
    }
    return SNAP_RETC_SUCCESS;
}

static snapu32_t action_extentop(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                mf_func_extentop_job_t job)
{
    return SNAP_RETC_FAILURE;
}

static snapu32_t action_bufmap(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                mf_func_bufmap_job_t job)
{
    return SNAP_RETC_FAILURE;
}

static snapu32_t process_action(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                action_reg * act_reg)
{
    snapu8_t function_code;
    function_code = act_reg->Data.function;

    switch(function_code)
    {
        case MF_FUNC_FILEMAP:
            return action_filemap(din_gmem, dout_gmem, act_reg->Data.jspec.filemap);
        case MF_FUNC_EXTENTOP:
            return action_extentop(din_gmem, dout_gmem, act_reg->Data.jspec.extentop);
        case MF_FUNC_BUFMAP:
            return action_bufmap(din_gmem, dout_gmem, act_reg->Data.jspec.bufmap);
        default:
            return SNAP_RETC_SUCCESS;
    }
}

// This design does use FPGA DDR.
// Need to set Environment Variable "SDRAM_USED=TRUE" before compilation.
void hls_action(snap_membus_t * din_gmem,
                snap_membus_t * dout_gmem,
                snap_membus_t * ddr_gmem,
                action_reg *action_reg, action_RO_config_reg *Action_Config)
{
    // Host Memory AXI Interface
#pragma HLS INTERFACE m_axi port=din_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE m_axi port=dout_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE s_axilite port=din_gmem bundle=ctrl_reg offset=0x030
#pragma HLS INTERFACE s_axilite port=dout_gmem bundle=ctrl_reg offset=0x040

    // Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=Action_Config
#pragma HLS INTERFACE s_axilite port=Action_Config bundle=ctrl_reg  offset=0x010
#pragma HLS DATA_PACK variable=action_reg
#pragma HLS INTERFACE s_axilite port=action_reg bundle=ctrl_reg offset=0x100
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_reg

    	// DDR memory Interface
#pragma HLS INTERFACE m_axi port=ddr_gmem bundle=card_mem0 offset=slave depth=512 \
  max_read_burst_length=64  max_write_burst_length=64 
#pragma HLS INTERFACE s_axilite port=ddr_gmem bundle=ctrl_reg offset=0x050

    /* Required Action Type Detection */
    switch (action_reg->Control.flags) {
        case 0:
        Action_Config->action_type = (snapu32_t)METALFPGA_ACTION_TYPE;
        Action_Config->release_level = (snapu32_t)HW_RELEASE_LEVEL;
        action_reg->Control.Retc = (snapu32_t)0xe00f;
        break;
        default:
        action_reg->Control.Retc = process_action(din_gmem, dout_gmem, action_reg);
        break;
    }
}

//-----------------------------------------------------------------------------
//--- TESTBENCH ---------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef NO_SYNTH

int main()
{
    static snap_membus_t din_gmem[1024];
    static snap_membus_t dout_gmem[1024];
    static snap_membus_t dram_gmem[1024];
    static snapu32_t nvme_gmem[];
    action_reg act_reg;
    action_RO_config_reg act_config;

    // read action config:
    act_reg.Control.flags = 0x0;
    hls_action(din_gmem, dout_gmem, &act_reg, &act_config);
    fprintf(stderr, "ACTION_TYPE:   %08x\nRELEASE_LEVEL: %08x\nRETC:          %04x\n",
        (unsigned int)act_config.action_type,
        (unsigned int)act_config.release_level,
        (unsigned int)act_reg.Control.Retc);


    // test action functions:

    /* fprintf(stderr, "// MODE_SET_KEY ciphertext 16 Byte at 0x100\n"); */
    /* act_reg.Control.flags = 0x1; */
    /* act_reg.Data.input_data.addr = 0; */
    /* act_reg.Data.data_length = 8; */
    /* act_reg.Data.mode = MODE_SET_KEY; */
    /* hls_action(din_gmem, dout_gmem, &act_reg, &act_config); */

    return 0;
}

#endif /* NO_SYNTH */
