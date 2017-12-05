#include <string.h>
#include <iostream>

#include "action_metalfpga.H"

#define HW_RELEASE_LEVEL       0x00000013

using namespace std;

static snapu64_t mf_extents_begin[MF_SLOT_COUNT][MF_EXTENT_COUNT];
static snapu64_t mf_extents_count[MF_SLOT_COUNT][MF_EXTENT_COUNT];
static snapu64_t mf_extents_prefixsum[MF_SLOT_COUNT][MF_EXTENT_COUNT];

static snapu32_t mf_fill_extents(mf_slot_offset slot, mf_extent_count_t extent_count,
                                 mf_extent_t direct_extents[6], snap_membus_t * din_gmem)
{
    mf_extent_count_t i_extent = 0;
    snapu64_t prefixsum = 0;
    for (; i_extent < 5; ++i_extent)
    {
        mf_extents_begin[slot][i_extent] = direct_extents[i_extent].begin;
        snapu64_t count = direct_extents[i_extent].count;
        prefixsum += count;
        mf_extents_count[slot][i_extent] = count;
        mf_extents_prefixsum[slot][i_extent] = prefixsum;

        if (i_extent == extent_count - 1)
        {
            return SNAP_RETC_SUCCESS;
        }
    }
    // i_extent == 5, extent_count > 5
    if (extent_count > 6) {
        //needs host
        snapu64_t extent_list_base = direct_extents[5].block_begin;
        snapu64_t host_block_offset = 0;
        snap_membus_t memory_block = din_gmem[extent_list_base>>ADDR_RIGHT_SHIFT]
        for(uint64_t host_offset = 0; i_extent < extent_count; ++i_extent, ++host_offset)
        {
            //TODO-lw:
            // local_offset = (i_extent - 5) % 4
            // if local_offset == 0: host_block_offset++; load next memory block
            // interpret memory_block((local_offset+1) * 64 - 1, local_offset * 64) as mf_extent_t (ENDIANNESS!)
            // calc prefixsum and store extent data
        }
        
    }
    else
    {
        mf_extents_begin[slot][i_extent] = direct_extents[i_extent].begin;
        snapu64_t count = direct_extents[i_extent].count;
        prefixsum += count;
        mf_extents_count[slot][i_extent] = count;
        mf_extents_prefixsum[slot][i_extent] = prefixsum;
    }
    return SNAP_RETC_SUCCESS;
}

// ------------------------------------------------
// --------------- ACTION FUNCTIONS ---------------
// ------------------------------------------------

static snapu32_t action_filemap(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                action_reg * action_reg)
{


}

static snapu32_t action_extentop(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                action_reg * action_reg)
{
}

static snapu32_t action_bufmap(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                action_reg * action_reg)
{
}

static snapu32_t process_action(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                action_reg * action_reg)
{
    snapu8_t function_code;
    function_code = action_reg->Data.function;

    switch(function_code)
    {
        case MF_FUNC_FILEMAP:
            return action_filemap(din_gmem, dout_gmem, act_reg);
        case MF_FUNC_EXTENTOP:
            return action_extentop(din_gmem, dout_gmem, act_reg);
        case MF_FUNC_EXTENTOP:
            return action_bufmap(din_gmem, dout_gmem, act_reg);
        default:
            return SNAP_RETC_SUCCESS;
    }
}

// This example doesn't use FPGA DDR.
// Need to set Environment Variable "SDRAM_USED=FALSE" before compilation.
void hls_action(snap_membus_t  *din_gmem, snap_membus_t  *dout_gmem,
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
