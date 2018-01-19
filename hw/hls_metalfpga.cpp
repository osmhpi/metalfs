#include <string.h>
#include <iostream>

#include "action_metalfpga.H"

#include "endianconv.hpp"
#include "hls_metalfpga_access.h"
#include "hls_metalfpga_map.h"


#define HW_RELEASE_LEVEL       0x00000013

using namespace std;

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
        if (!mf_map_close(slot))
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

        if (job.flags & MF_FILEMAP_INDIRECT)
        {
            if (!mf_map_open_direct(slot, extent_count, job.extents.direct))
            {
                mf_map_close(slot);
                return SNAP_RETC_FAILURE;
            }
        }
        else
        {
            if (!mf_map_open_indirect(slot, extent_count, job.extents.indirect_address, din_gmem))
            {
                mf_map_close(slot);
                return SNAP_RETC_FAILURE;
            }
        }
    }
    else if ((job.flags & MF_FILEMAP_MODE) == MF_FILEMAP_MODE_TEST)
    {
        snap_membus_t line = din_gmem[job.extents.indirect_address>>ADDR_RIGHT_SHIFT];
        snapu64_t logical_block_number = mf_get64(line, 0);
        snapu64_t physical_block_number = mf_map_get_pblock(slot, logical_block_number);
        mf_set64(line, 0, physical_block_number);
        dout_gmem[job.extents.indirect_address>>ADDR_RIGHT_SHIFT] = line;
    }
    else
    {
        return SNAP_RETC_FAILURE;
    }
    return SNAP_RETC_SUCCESS;
}

static snapu32_t action_access(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                mf_func_extentop_job_t job)
{
    return SNAP_RETC_FAILURE;
}

static snapu32_t process_action(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                mf_func_access_job_t job)
{
    snapu8_t function_code;
    function_code = act_reg->Data.function;

    switch(function_code)
    {
        case MF_FUNC_FILEMAP:
            return action_filemap(din_gmem, dout_gmem, act_reg->Data.jspec.filemap);
        case MF_FUNC_ACCESS:
            return action_extentop(din_gmem, dout_gmem, act_reg->Data.jspec.extentop);
        default:
            return SNAP_RETC_FAILURE;
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
