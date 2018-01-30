#include "action_metalfpga.H"

#include "mf_endian.h"
#include "mf_file.h"
#include "mf_jobstruct.h"


/* #include "hls_globalmem.h" */

/* snap_membus_t * gmem_host_in; */
/* snap_membus_t * gmem_host_out; */
/* snap_membus_t * gmem_ddr; */


#define HW_RELEASE_LEVEL       0x00000013


static mf_retc_t process_action(snap_membus_t * mem_in,
                                snap_membus_t * mem_out,
                                action_reg * act_reg);

static mf_retc_t action_map(snap_membus_t * mem_in, const mf_job_map_t & job);
static mf_retc_t action_query(mf_job_query_t & job);
static mf_retc_t action_access(const mf_job_access_t & job);

static mf_retc_t action_file_write_buffer(snap_membus_t * mem_in,
                                      snap_membus_t * mem_ddr,
                                      const mf_job_access_t & job);
static mf_retc_t action_file_read_buffer(snap_membus_t * mem_out,
                                     snap_membus_t * mem_ddr,
                                     const mf_job_access_t & job);

static void action_file_read_block(snap_membus_t * mem,
                                   mf_slot_offset_t slot,
                                   snapu64_t buffer_address,
                                   mf_block_offset_tbegin_offset,
                                   mf_block_offset_t end_offset);
static void action_file_write_block(snap_membus_t * mem,
                                    mf_slot_offset_t slot,
                                    snapu64_t buffer_address,
                                    mf_block_offset_tbegin_offset,
                                    mf_block_offset_t end_offset);
// ------------------------------------------------
// -------------- ACTION ENTRY POINT --------------
// ------------------------------------------------
// This design uses FPGA DDR.
// Set Environment Variable "SDRAM_USED=TRUE" before compilation.
void hls_action(snap_membus_t * din,
                snap_membus_t * dout,
                snap_membus_t * ddr,
                action_reg * action_reg,
                action_RO_config_reg * action_config)
{
    // Configure Host Memory AXI Interface
#pragma HLS INTERFACE m_axi port=din bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE m_axi port=dout bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE s_axilite port=din bundle=ctrl_reg offset=0x030
#pragma HLS INTERFACE s_axilite port=dout bundle=ctrl_reg offset=0x040

    // Configure Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=action_config
#pragma HLS INTERFACE s_axilite port=action_config bundle=ctrl_reg  offset=0x010
#pragma HLS DATA_PACK variable=action_reg
#pragma HLS INTERFACE s_axilite port=action_reg bundle=ctrl_reg offset=0x100
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_reg

    // Configure DDR memory Interface
#pragma HLS INTERFACE m_axi port=ddr bundle=card_mem0 offset=slave depth=512 \
  max_read_burst_length=64  max_write_burst_length=64 
#pragma HLS INTERFACE s_axilite port=ddr bundle=ctrl_reg offset=0x050


    // Make memory ports globally accessible
    /* gmem_host_in = din; */
    /* gmem_host_out = dout; */
    /* gmem_ddr = ddr; */

    // Required Action Type Detection
    switch (action_reg->Control.flags) {
    case 0:
        action_config->action_type = (snapu32_t)METALFPGA_ACTION_TYPE;
        action_config->release_level = (snapu32_t)HW_RELEASE_LEVEL;
        action_reg->Control.Retc = (snapu32_t)0xe00f;
        break;
    default:
        action_reg->Control.Retc = process_action(din, dout, action_reg);
        break;
    }
}


// ------------------------------------------------
// --------------- ACTION FUNCTIONS ---------------
// ------------------------------------------------

// Decode job_type and call appropriate action
static mf_retc_t process_action(snap_membus_t * mem_in,
                                snap_membus_t * mem_out,
                                action_reg * act_reg)
{
    switch(act_reg->Data.job_type)
    {
        case MF_JOB_MAP:
          {
            mf_job_map_t map_job = mf_read_job_map(mem_in, act_reg->Data.job_address);
            return action_map(mem_in, map_job);
          }
        case MF_JOB_QUERY:
          {
            mf_job_query_t query_job = mf_read_job_query(mem_in, act_reg->Data.job_address);
            mf_retc_t retc = action_query(query_job);
            mf_write_job_query(mem_out, act_reg->Data.job_address, query_job);
            return retc;
          }
        case MF_JOB_ACCESS:
          {
            mf_job_access_t access_job = mf_read_job_access(mem_in, act_reg->Data.job_address);
            return action_access(access_job);
          }
        default:
            return SNAP_RETC_FAILURE;
    }
}

// File Map / Unmap Operation:
static mf_retc_t action_map(snap_membus_t * mem_in,
                            const mf_job_map_t & job)
{
    if (job.slot >= MF_SLOT_COUNT)
    {
        return SNAP_RETC_FAILURE;
    }
    mf_slot_offset_t slot = job.slot;

    if (job.map_else_unmap)
    {
        if (job.extent_count > MF_EXTENT_COUNT)
        {
            return SNAP_RETC_FAILURE;
        }
        mf_extent_count_t extent_count = job.extent_count;

        if (!mf_file_open(slot, extent_count, job.extent_address, mem_in))
        {
            mf_file_close(slot);
            return SNAP_RETC_FAILURE;
        }
    }
    else
    {
        if (!mf_file_close(slot))
        {
            return SNAP_RETC_FAILURE;
        }
    }
    return SNAP_RETC_SUCCESS;
}

static mf_retc_t action_query(mf_job_query_t & job)
{
    snap_membus_t line;
    if (job.query_mapping)
    {
        job.lblock_to_pblock = mf_file_map_pblock(job.slot, job.lblock_to_pblock);
    }
    if (job.query_state)
    {
        job.is_open = mf_file_is_open(job.slot)? MF_TRUE : MF_FALSE;
        job.is_active = mf_file_is_active(job.slot)? MF_TRUE : MF_FALSE;
        job.extent_count = mf_file_get_extent_count(job.slot);
        job.block_count = mf_file_get_block_count(job.slot);
        job.current_lblock = mf_file_get_lblock(job.slot);
        job.current_pblock = mf_file_get_pblock(job.slot);
    }
    return SNAP_RETC_SUCCESS;
}

static mf_retc_t action_access(snap_membus_t * mem_in,
                               snap_membus_t * mem_out,
                               snap_membus_t * mem_ddr,
                               const mf_job_access_t & job)
{
    if (! mf_file_is_open(job.slot))
    {
        return SNAP_RETC_FAILURE;
    }

    if (job.write_else_read)
    {
        return mf_file_write_buffer(mem_in, mem_ddr, job);
    }
    else
    {
        return mf_file_read_buffer(mem_out, mem_ddr, job);
    }
}

static mf_retc_t action_file_write_buffer(snap_membus_t * mem_in,
                                   snap_membus_t * mem_ddr,
                                   const mf_job_access_t & job)
{
    const snapu64_t file_blocks = mf_file_get_block_count(job.slot);
    const snapu64_t file_bytes = file_blocks * MF_BLOCK_BYTES;
    if (job.file_byte_offset + job.file_byte_count > file_bytes)
    {
        return SNAP_RETC_FAILURE;
    }

    const mf_block_offset_t file_begin_offset = MF_BLOCK_OFFSET(job.file_byte_offset);
    const snapu64_t file_begin_block = MF_BLOCK_NUMBER(job.file_byte_offset);
    const mf_block_offset_t file_end_offset = MF_BLOCK_OFFSET(job.file_byte_offset + job.file_byte_count - 1);
    const snapu64_t file_end_block = MF_BLOCK_NUMBER(job.file_byte_offset + job.file_byte_count - 1);

    snapu64_t buffer_address = job.buffer_address;
    mf_file_seek(mem_ddr, job.slot, file_begin_block, MF_FALSE);//TODO-lw return FAILURE if fails
    for (snapu64_t i_block = file_begin_block; i_block <= file_end_block; ++i_block)
    {
        mf_block_offset_t begin_offset = (i_block == file_begin_block)? file_begin_offset : 0;
        mf_block_offset_t end_offset = (i_block == file_end_block)? file_end_offset : MF_BLOCK_BYTES - 1;
        action_file_write_block(mem_in, job.slot, buffer_address, begin_offset, end_offset);

        mf_file_next(mem_ddr, job.slot, MF_TRUE);//TODO-lw return FAILURE if fails
    }
    return SNAP_RETC_SUCCESS;
}

static mf_retc_t action_file_read_buffer(snap_membus_t * mem_out,
                                  snap_membus_t * mem_ddr,
                                  const mf_job_access_t & job)
{

    return SNAP_RETC_FAILURE;
}

static void action_file_write_block(snap_membus_t * mem,
                                    mf_slot_offset_t slot,
                                    snapu64_t buffer_address,
                                    mf_block_offset_tbegin_offset,
                                    mf_block_offset_t end_offset)
{
    mf_file_buffers[slot];
}

static void action_file_read_block(snap_membus_t * mem,
                                   mf_slot_offset_t slot,
                                   snapu64_t buffer_address,
                                   mf_block_offset_tbegin_offset,
                                   mf_block_offset_t end_offset)
{
    mf_file_buffers[slot];
}
//-----------------------------------------------------------------------------
//--- TESTBENCH ---------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef NO_SYNTH

#include <stdio.h>

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
