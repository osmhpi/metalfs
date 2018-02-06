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
                                snap_membus_t * mem_ddr,
                                action_reg * act_reg);

static mf_retc_t action_map(snap_membus_t * mem_in, const mf_job_map_t & job);
static mf_retc_t action_query(mf_job_query_t & job);
static mf_retc_t action_access(snap_membus_t * mem_in,
                               snap_membus_t * mem_out,
                               snap_membus_t * mem_ddr,
                               const mf_job_access_t & job);

static void action_file_write_block(snap_membus_t * mem_in,
                                   mf_slot_offset_t slot,
                                   snapu64_t buffer_address,
                                   mf_block_offset_t begin_offset,
                                   mf_block_offset_t end_offset);
static void action_file_read_block(snap_membus_t * mem_in,
                                    snap_membus_t * mem_out,
                                    mf_slot_offset_t slot,
                                    snapu64_t buffer_address,
                                    mf_block_offset_t begin_offset,
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
        action_reg->Control.Retc = process_action(din, dout, ddr, action_reg);
        break;
    }
}


// ------------------------------------------------
// --------------- ACTION FUNCTIONS ---------------
// ------------------------------------------------

// Decode job_type and call appropriate action
static mf_retc_t process_action(snap_membus_t * mem_in,
                                snap_membus_t * mem_out,
                                snap_membus_t * mem_ddr,
                                action_reg * act_reg)
{
    switch(act_reg->Data.job_type)
    {
        case MF_JOB_MAP:
          {
            mf_job_map_t map_job = mf_read_job_map(mem_in, act_reg->Data.job_address);
            return action_map(mem_in, map_job);
          }
          break;
        case MF_JOB_QUERY:
          {
            mf_job_query_t query_job = mf_read_job_query(mem_in, act_reg->Data.job_address);
            mf_retc_t retc = action_query(query_job);
            mf_write_job_query(mem_out, act_reg->Data.job_address, query_job);
            return retc;
          }
          break;
        case MF_JOB_ACCESS:
          {
            mf_job_access_t access_job = mf_read_job_access(mem_in, act_reg->Data.job_address);
            return action_access(mem_in, mem_out, mem_ddr, access_job);
          }
          break;
        default:
            return SNAP_RETC_FAILURE;
          break;
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
    if (! mf_file_seek(mem_ddr, job.slot, file_begin_block, MF_FALSE)) return SNAP_RETC_FAILURE;
    for (snapu64_t i_block = file_begin_block; i_block <= file_end_block; ++i_block)
    {
        /* mf_block_offset_t begin_offset = (i_block == file_begin_block)? file_begin_offset : 0; */
        /* mf_block_offset_t end_offset = (i_block == file_end_block)? file_end_offset : MF_BLOCK_BYTES - 1; */
        mf_block_offset_t begin_offset = 0;
        if (i_block == file_begin_block)
        {
            begin_offset = file_begin_offset;
        }
        mf_block_offset_t end_offset = MF_BLOCK_BYTES - 1;
        if (i_block == file_end_block)
        {
            end_offset = file_end_offset;
        }

        if (job.write_else_read)
        {
            action_file_write_block(mem_in, job.slot, buffer_address, begin_offset, end_offset);
        }
        else
        {
            action_file_read_block(mem_in, mem_out, job.slot, buffer_address, begin_offset, end_offset);
        }

        buffer_address += end_offset - begin_offset + 1;

        if (i_block == file_end_block)
        {
            if (! mf_file_flush(mem_ddr, job.slot)) return SNAP_RETC_FAILURE;
        }
        else
        {
            if (! mf_file_next(mem_ddr, job.slot, MF_TRUE)) return SNAP_RETC_FAILURE;
        }
    }
    return SNAP_RETC_SUCCESS;
}


static void action_file_write_block(snap_membus_t * mem_in,
                                    mf_slot_offset_t slot,
                                    snapu64_t buffer_address,
                                    mf_block_offset_t begin_offset,
                                    mf_block_offset_t end_offset)
{
    snapu64_t end_address = buffer_address + end_offset - begin_offset;

    snapu64_t begin_line = MFB_ADDRESS(buffer_address);
    mfb_byteoffset_t begin_line_offset = MFB_LINE_OFFSET(buffer_address);
    snapu64_t end_line = MFB_ADDRESS(end_address);
    mfb_byteoffset_t end_line_offset = MFB_LINE_OFFSET(end_address);

    mf_block_offset_t offset = begin_offset;
    for (snapu64_t i_line = begin_line; i_line <= end_line; ++i_line)
    {
        snap_membus_t line = mem_in[i_line];
        mfb_bytecount_t begin_byte = 0;
        if (i_line == begin_line)
        {
            begin_byte = begin_line_offset;
        }
        mfb_bytecount_t end_byte = MFB_INCREMENT - 1;
        if (i_line == end_line)
        {
            end_byte = end_line_offset;
        }
        for (mfb_bytecount_t i_byte = begin_byte; i_byte <= end_byte; ++i_byte)
        {
            mf_file_buffers[slot][offset++] = mf_get8(line, i_byte);
        }
    }
}

static void action_file_read_block(snap_membus_t * mem_in,
                                   snap_membus_t * mem_out,
                                   mf_slot_offset_t slot,
                                   snapu64_t buffer_address,
                                   mf_block_offset_t begin_offset,
                                   mf_block_offset_t end_offset)
{
    snapu64_t end_address = buffer_address + end_offset - begin_offset;

    snapu64_t begin_line = MFB_ADDRESS(buffer_address);
    mfb_byteoffset_t begin_line_offset = MFB_LINE_OFFSET(buffer_address);
    snapu64_t end_line = MFB_ADDRESS(end_address);
    mfb_byteoffset_t end_line_offset = MFB_LINE_OFFSET(end_address);

    mf_block_offset_t offset = begin_offset;
    for (snapu64_t i_line = begin_line; i_line <= end_line; ++i_line)
    {
        snap_membus_t line = mem_in[i_line];
        mfb_bytecount_t begin_byte = 0;
        if (i_line == begin_line)
        {
            begin_byte = begin_line_offset;
        }
        mfb_bytecount_t end_byte = MFB_INCREMENT - 1;
        if (i_line == end_line)
        {
            end_byte = end_line_offset;
        }
        for (mfb_bytecount_t i_byte = begin_byte; i_byte <= end_byte; ++i_byte)
        {
            mf_set8(line, i_byte, mf_file_buffers[slot][offset++]);
        }
        mem_out[i_line] = line;
    }
}
//-----------------------------------------------------------------------------
//--- TESTBENCH ---------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef NO_SYNTH

#include <stdio.h>

int main()
{
    static snap_membus_t host_gmem[1024];
    static snap_membus_t dram_gmem[1024];
    //static snapu32_t nvme_gmem[];
    action_reg act_reg;
    action_RO_config_reg act_config;

    // read action config:
    act_reg.Control.flags = 0x0;
    hls_action(host_gmem, host_gmem, dram_gmem, &act_reg, &act_config);
    fprintf(stderr, "ACTION_TYPE:   %08x\nRELEASE_LEVEL: %08x\nRETC:          %04x\n",
        (unsigned int)act_config.action_type,
        (unsigned int)act_config.release_level,
        (unsigned int)act_reg.Control.Retc);


    // test action functions:

    fprintf(stderr, "// MAP slot 2 1000:7,2500:3,1700:8\n");
    uint64_t * job_mem = (uint64_t *)host_gmem;
    uint8_t * job_mem_b = (uint8_t *)host_gmem;
    job_mem_b[0] = 2; // slot
    job_mem[1] = true; // map
    job_mem[2] = 3; // extent_count

    job_mem[8]  = 1000; // ext0.begin
    job_mem[9]  = 7;    // ext0.count
    job_mem[10] = 2500; // ext1.begin
    job_mem[11] = 3;    // ext1.count
    job_mem[12] = 1700; // ext2.begin
    job_mem[13] = 8;    // ext2.count
    
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_MAP;
    hls_action(host_gmem, host_gmem, dram_gmem, &act_reg, &act_config);

    fprintf(stderr, "// MAP slot 7 1200:8,1500:24\n");
	job_mem_b[0] = 7; // slot
	job_mem[1] = true; // map
	job_mem[2] = 2; // extent_count

	job_mem[8]  = 1200; // ext0.begin
	job_mem[9]  = 8;    // ext0.count
	job_mem[10] = 1500; // ext1.begin
	job_mem[11] = 24;   // ext1.count

	act_reg.Control.flags = 0x1;
	act_reg.Data.job_address = 0;
	act_reg.Data.job_type = MF_JOB_MAP;
	hls_action(host_gmem, host_gmem, dram_gmem, &act_reg, &act_config);

    fprintf(stderr, "// QUERY slot 2, lblock 9\n");

    job_mem_b[0] = 2; // slot
    job_mem_b[1] = true; // query_mapping
    job_mem_b[2] = true; // query_state
	job_mem[1] = 9; // lblock

	act_reg.Control.flags = 0x1;
	act_reg.Data.job_address = 0;
	act_reg.Data.job_type = MF_JOB_QUERY;
	hls_action(host_gmem, host_gmem, dram_gmem, &act_reg, &act_config);

	uint64_t is_open = mf_file_is_open(2)? MF_TRUE : MF_FALSE;
	uint64_t is_active = mf_file_is_active(2)? MF_TRUE : MF_FALSE;
	uint64_t extent_count = mf_file_get_extent_count(2);
	uint64_t block_count = mf_file_get_block_count(2);
	uint64_t current_lblock = mf_file_get_lblock(2);
	uint64_t current_pblock = mf_file_get_pblock(2);

    
    fprintf(stderr, "// ACCESS slot 2, write 6K @ offset 3K\n");
    job_mem_b[0] = 2; // slot
    job_mem_b[1] = true; // write
    job_mem[1] = 128; // buffer beginning at mem[2]
    job_mem[2] = 3072; // 3K offset
    job_mem[3] = 6144; // 6K length

    snap_membus_t data_line = 0;
    mf_set64(data_line, 0, 0xda7a111100000001ull);
    mf_set64(data_line, 8, 0xda7a222200000002ull);
    mf_set64(data_line, 16, 0xda7a333300000003ull);
    mf_set64(data_line, 24, 0xda7a444400000004ull);
    mf_set64(data_line, 32, 0xda7a555500000005ull);
    mf_set64(data_line, 40, 0xda7a666600000006ull);
    mf_set64(data_line, 48, 0xda7a777700000007ull);
    mf_set64(data_line, 56, 0xda7a888800000008ull);
    for (int i = 0; i < 96; ++i) {
        host_gmem[2+i] = data_line;
    }

	act_reg.Control.flags = 0x1;
	act_reg.Data.job_address = 0;
	act_reg.Data.job_type = MF_JOB_ACCESS;
	hls_action(host_gmem, host_gmem, dram_gmem, &act_reg, &act_config);

    fprintf(stderr, "// ACCESS slot 2, read 100 @ offset 3077\n");
    job_mem_b[0] = 2; // slot
    job_mem_b[1] = false; // read
    job_mem[1] = 7168; // buffer beginning at mem[112]
    job_mem[2] = 3077; // 3077 offset
    job_mem[3] = 100; // 100 length

	act_reg.Control.flags = 0x1;
	act_reg.Data.job_address = 0;
	act_reg.Data.job_type = MF_JOB_ACCESS;
	hls_action(host_gmem, host_gmem, dram_gmem, &act_reg, &act_config);
    return 0;
}

#endif /* NO_SYNTH */
