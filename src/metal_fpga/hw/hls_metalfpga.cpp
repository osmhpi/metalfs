#include "action_metalfpga.H"

#include "mf_endian.h"
#include "mf_file.h"
#include "mf_jobstruct.h"
#include "mf_stream.h"

#include "mf_afu_mem.h"
#include "mf_afu_file.h"
#include "mf_afu_passthrough.h"
#include "mf_afu_change_case.h"

#define HW_RELEASE_LEVEL       0x00000013

#define DRAM_ENABLED
#define NVME_ENABLED


static mf_retc_t process_action(snap_membus_t * mem_in,
                                snap_membus_t * mem_out,
#ifdef DRAM_ENABLED
                                snap_membus_t * mem_ddr_in,
                                snap_membus_t * mem_ddr_out,
#endif
#ifdef NVME_ENABLED
                                snapu32_t * nvme,
#endif
                                mf_stream &axis_s_0,
                                mf_stream &axis_s_1,
                                mf_stream &axis_s_2,
                                mf_stream &axis_s_3,
                                mf_stream &axis_s_4,
                                mf_stream &axis_s_5,
                                mf_stream &axis_s_6,
                                mf_stream &axis_s_7,
                                mf_stream &axis_m_0,
                                mf_stream &axis_m_1,
                                mf_stream &axis_m_2,
                                mf_stream &axis_m_3,
                                mf_stream &axis_m_4,
                                mf_stream &axis_m_5,
                                mf_stream &axis_m_6,
                                mf_stream &axis_m_7,
                                snapu32_t *switch_ctrl,
                                action_reg * act_reg);

static mf_retc_t action_map(snap_membus_t * mem_in, const mf_job_map_t & job);
// static mf_retc_t action_query(snap_membus_t * mem_out, uint64_t job_address, mf_job_query_t & job);
// static mf_retc_t action_access(snap_membus_t * mem_in,
//                                snap_membus_t * mem_out,
//                                snap_membus_t * mem_ddr,
//                                const mf_job_access_t & job);
static mf_retc_t action_mount(snapu32_t * nvme, const mf_job_fileop_t & job);
static mf_retc_t action_writeback(snapu32_t * nvme, const mf_job_fileop_t & job);

static mf_retc_t action_configure_streams(snapu32_t *switch_ctrl, snap_membus_t * mem_out, const uint64_t job_address);
static mf_retc_t action_run_afus(
    snap_membus_t * mem_in,
    snap_membus_t * mem_out,
#ifdef DRAM_ENABLED
    snap_membus_t * mem_ddr_in,
    snap_membus_t * mem_ddr_out,
#endif
    mf_stream &axis_s_0,
    mf_stream &axis_s_1,
    mf_stream &axis_s_2,
    mf_stream &axis_s_3,
    mf_stream &axis_s_4,
    mf_stream &axis_s_5,
    mf_stream &axis_s_6,
    mf_stream &axis_s_7,
    mf_stream &axis_m_0,
    mf_stream &axis_m_1,
    mf_stream &axis_m_2,
    mf_stream &axis_m_3,
    mf_stream &axis_m_4,
    mf_stream &axis_m_5,
    mf_stream &axis_m_6,
    mf_stream &axis_m_7
);

// static void action_file_write_block(snap_membus_t * mem_in,
//                                    mf_slot_offset_t slot,
//                                    snapu64_t buffer_address,
//                                    mf_block_offset_t begin_offset,
//                                    mf_block_offset_t end_offset);
// static void action_file_read_block(snap_membus_t * mem_in,
//                                     snap_membus_t * mem_out,
//                                     snapu64_t slot,
//                                     snapu64_t buffer_address,
//                                     mf_block_offset_t begin_offset,
//                                     mf_block_offset_t end_offset);

// ------------------------------------------------
// -------------- ACTION ENTRY POINT --------------
// ------------------------------------------------
// This design uses FPGA DDR.
// Set Environment Variable "SDRAM_USED=TRUE" before compilation.
void hls_action(snap_membus_t * din,
                snap_membus_t * dout,
#ifdef DRAM_ENABLED
                snap_membus_t * ddrin,
                snap_membus_t * ddrout,
#endif
#ifdef NVME_ENABLED
                snapu32_t * nvme,
#endif
                mf_stream &axis_s_0,
                mf_stream &axis_s_1,
                mf_stream &axis_s_2,
                mf_stream &axis_s_3,
                mf_stream &axis_s_4,
                mf_stream &axis_s_5,
                mf_stream &axis_s_6,
                mf_stream &axis_s_7,
                mf_stream &axis_m_0,
                mf_stream &axis_m_1,
                mf_stream &axis_m_2,
                mf_stream &axis_m_3,
                mf_stream &axis_m_4,
                mf_stream &axis_m_5,
                mf_stream &axis_m_6,
                mf_stream &axis_m_7,
                snapu32_t *switch_ctrl,
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

#ifdef DRAM_ENABLED
    // Configure DDR memory Interface
#pragma HLS INTERFACE m_axi port=ddrin bundle=card_mem0 offset=slave depth=512 \
    max_read_burst_length=64  max_write_burst_length=64
#pragma HLS INTERFACE s_axilite port=ddrin bundle=ctrl_reg offset=0x050
#pragma HLS INTERFACE m_axi port=ddrout bundle=card_mem0 offset=slave depth=512 \
    max_read_burst_length=64  max_write_burst_length=64
#pragma HLS INTERFACE s_axilite port=ddrout bundle=ctrl_reg offset=0x070
#endif

#ifdef NVME_ENABLED
    //NVME Config Interface
#pragma HLS INTERFACE m_axi port=nvme bundle=nvme //offset=slave
//#pragma HLS INTERFACE s_axilite port=nvme bundle=ctrl_reg offset=0x060
#endif

    // Configure AXI4 Stream Interface
#pragma HLS INTERFACE axis port=axis_s_0
#pragma HLS INTERFACE axis port=axis_s_1
#pragma HLS INTERFACE axis port=axis_s_2
#pragma HLS INTERFACE axis port=axis_s_3
#pragma HLS INTERFACE axis port=axis_s_4
#pragma HLS INTERFACE axis port=axis_s_5
#pragma HLS INTERFACE axis port=axis_s_6
#pragma HLS INTERFACE axis port=axis_s_7
#pragma HLS INTERFACE axis port=axis_m_0
#pragma HLS INTERFACE axis port=axis_m_1
#pragma HLS INTERFACE axis port=axis_m_2
#pragma HLS INTERFACE axis port=axis_m_3
#pragma HLS INTERFACE axis port=axis_m_4
#pragma HLS INTERFACE axis port=axis_m_5
#pragma HLS INTERFACE axis port=axis_m_6
#pragma HLS INTERFACE axis port=axis_m_7
#pragma HLS INTERFACE m_axi port=switch_ctrl bundle=switch_ctrl_reg offset=0x44A00000


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
        action_reg->Control.Retc = process_action(
            din,
            dout,
#ifdef DRAM_ENABLED
            ddrin,
            ddrout,
#endif
#ifdef NVME_ENABLED
            nvme,
#endif
            axis_s_0,
            axis_s_1,
            axis_s_2,
            axis_s_3,
            axis_s_4,
            axis_s_5,
            axis_s_6,
            axis_s_7,
            axis_m_0,
            axis_m_1,
            axis_m_2,
            axis_m_3,
            axis_m_4,
            axis_m_5,
            axis_m_6,
            axis_m_7,
            switch_ctrl,
            action_reg);
        break;
    }
}


// ------------------------------------------------
// --------------- ACTION FUNCTIONS ---------------
// ------------------------------------------------

// Decode job_type and call appropriate action
static mf_retc_t process_action(snap_membus_t * mem_in,
                                snap_membus_t * mem_out,
#ifdef DRAM_ENABLED
                                snap_membus_t * mem_ddr_in,
                                snap_membus_t * mem_ddr_out,
#endif
#ifdef NVME_ENABLED
                                snapu32_t * nvme,
#endif
                                mf_stream &axis_s_0,
                                mf_stream &axis_s_1,
                                mf_stream &axis_s_2,
                                mf_stream &axis_s_3,
                                mf_stream &axis_s_4,
                                mf_stream &axis_s_5,
                                mf_stream &axis_s_6,
                                mf_stream &axis_s_7,
                                mf_stream &axis_m_0,
                                mf_stream &axis_m_1,
                                mf_stream &axis_m_2,
                                mf_stream &axis_m_3,
                                mf_stream &axis_m_4,
                                mf_stream &axis_m_5,
                                mf_stream &axis_m_6,
                                mf_stream &axis_m_7,
                                snapu32_t *switch_ctrl,
                                action_reg * act_reg)
{
    if (act_reg->Data.job_type == MF_JOB_MAP)
    {
        mf_job_map_t map_job = mf_read_job_map(mem_in, act_reg->Data.job_address);
        return action_map(mem_in, map_job);
    }
    // else if (act_reg->Data.job_type == MF_JOB_QUERY)
    // {
    //     mf_job_query_t query_job = mf_read_job_query(mem_in, act_reg->Data.job_address);
    //     mf_retc_t retc = action_query(mem_out, act_reg->Data.job_address, query_job);
    //     return retc;
    // }
    // else if (act_reg->Data.job_type == MF_JOB_ACCESS)
    // {
    //     mf_job_access_t access_job = mf_read_job_access(mem_in, act_reg->Data.job_address);
    //     return action_access(mem_in, mem_out, mem_ddr_in, access_job);
    // }
    else if (act_reg->Data.job_type == MF_JOB_MOUNT)
    {
#ifdef NVME_ENABLED
        mf_job_fileop_t mount_job = mf_read_job_fileop(mem_in, act_reg->Data.job_address);
        return action_mount(nvme, mount_job);
#else
        return SNAP_RETC_SUCCESS;
#endif
    }
    else if (act_reg->Data.job_type == MF_JOB_WRITEBACK)
    {
#ifdef NVME_ENABLED
        mf_job_fileop_t writeback_job = mf_read_job_fileop(mem_in, act_reg->Data.job_address);
        return action_writeback(nvme, writeback_job);
#else
        return SNAP_RETC_SUCCESS;
#endif
    }
    else if (act_reg->Data.job_type == MF_JOB_CONFIGURE_STREAMS)
    {
        return action_configure_streams(switch_ctrl, mem_in, act_reg->Data.job_address);
    }
    else if (act_reg->Data.job_type == MF_JOB_RUN_AFUS)
    {
        return action_run_afus(
            mem_in,
            mem_out,
#ifdef DRAM_ENABLED
            mem_ddr_in,
            mem_ddr_out,
#endif
            axis_s_0,
            axis_s_1,
            axis_s_2,
            axis_s_3,
            axis_s_4,
            axis_s_5,
            axis_s_6,
            axis_s_7,
            axis_m_0,
            axis_m_1,
            axis_m_2,
            axis_m_3,
            axis_m_4,
            axis_m_5,
            axis_m_6,
            axis_m_7
        );
    }
    else if (act_reg->Data.job_type == MF_JOB_AFU_MEM_SET_READ_BUFFER)
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        return afu_mem_set_config(mf_get64<0>(line), mf_get64<8>(line), read_mem_config);
    }
    else if (act_reg->Data.job_type == MF_JOB_AFU_MEM_SET_WRITE_BUFFER)
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        return afu_mem_set_config(mf_get64<0>(line), mf_get64<8>(line), write_mem_config);
    }
    else if (act_reg->Data.job_type == MF_JOB_AFU_MEM_SET_DRAM_READ_BUFFER)
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        return afu_mem_set_config(mf_get64<0>(line), mf_get64<8>(line), read_ddr_mem_config);
    }
    else if (act_reg->Data.job_type == MF_JOB_AFU_MEM_SET_DRAM_WRITE_BUFFER)
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        return afu_mem_set_config(mf_get64<0>(line), mf_get64<8>(line), write_ddr_mem_config);
    }
    else if (act_reg->Data.job_type == MF_JOB_AFU_CHANGE_CASE_SET_MODE)
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        return afu_change_case_set_mode(mf_get64<0>(line));
    }
    return SNAP_RETC_FAILURE;
}

// File Map / Unmap Operation:
static mf_retc_t action_map(snap_membus_t * mem_in,
                            const mf_job_map_t & job)
{
    switch (job.slot) {
        case 0: {
            mf_extmap_load(read_extmap, job.extent_count, job.extent_address, mem_in);
            return SNAP_RETC_SUCCESS;
        }
        case 1: {
            mf_extmap_load(write_extmap, job.extent_count, job.extent_address, mem_in);
            return SNAP_RETC_SUCCESS;
        }
        default: {
            return SNAP_RETC_FAILURE;
        }
    }
}

// static mf_retc_t action_query(snap_membus_t * mem_out, const uint64_t job_address, mf_job_query_t & job)
// {
//     snap_membus_t line;
//     snapu64_t lblk = job.lblock_to_pblock;
//     if (job.query_mapping)
//     {
//         job.lblock_to_pblock = mf_file_map_pblock(job.slot, job.lblock_to_pblock);
//     }
//     if (job.query_state)
//     {
//         job.is_open = mf_file_is_open(job.slot)? MF_TRUE : MF_FALSE;
//         job.is_active = mf_file_is_active(job.slot)? MF_TRUE : MF_FALSE;
//         job.extent_count = mf_file_get_extent_count(job.slot);
//         job.block_count = mf_file_get_block_count(job.slot);
//         job.current_lblock = mf_file_get_lblock(job.slot);
//         job.current_pblock = mf_file_get_pblock(job.slot);
//     }
//     mf_write_job_query(mem_out, job_address, job);
//     return SNAP_RETC_SUCCESS;
// }

static mf_retc_t action_mount(snapu32_t * nvme, const mf_job_fileop_t & job)
{
    switch (job.slot) {
        case 0: {
            mf_file_load_buffer(nvme, read_extmap, job.file_offset, job.dram_offset, job.length);
            return SNAP_RETC_SUCCESS;
        }
        case 1: {
            mf_file_load_buffer(nvme, write_extmap, job.file_offset, job.dram_offset, job.length);
            return SNAP_RETC_SUCCESS;
        }
        default: {
            return SNAP_RETC_FAILURE;
        }
    }
}

static mf_retc_t action_writeback(snapu32_t * nvme, const mf_job_fileop_t & job)
{
    switch (job.slot) {
        case 0: {
            mf_file_write_buffer(nvme, read_extmap, job.file_offset, job.dram_offset, job.length);
            return SNAP_RETC_SUCCESS;
        }
        case 1: {
            mf_file_write_buffer(nvme, write_extmap, job.file_offset, job.dram_offset, job.length);
            return SNAP_RETC_SUCCESS;
        }
        default: {
            return SNAP_RETC_FAILURE;
        }
    }
}

// static mf_retc_t action_access(snap_membus_t * mem_in,
//                                snap_membus_t * mem_out,
//                                snap_membus_t * mem_ddr,
//                                const mf_job_access_t & job)
// {
//     if (! mf_file_is_open(job.slot))
//     {
//         return SNAP_RETC_FAILURE;
//     }

//     const snapu64_t file_blocks = mf_file_get_block_count(job.slot);
//     const snapu64_t file_bytes = file_blocks * MF_BLOCK_BYTES;
//     if (job.file_byte_offset + job.file_byte_count > file_bytes)
//     {
//         return SNAP_RETC_FAILURE;
//     }

//     const mf_block_offset_t file_begin_offset = MF_BLOCK_OFFSET(job.file_byte_offset);
//     const snapu64_t file_begin_block = MF_BLOCK_NUMBER(job.file_byte_offset);
//     const mf_block_offset_t file_end_offset = MF_BLOCK_OFFSET(job.file_byte_offset + job.file_byte_count - 1);
//     const snapu64_t file_end_block = MF_BLOCK_NUMBER(job.file_byte_offset + job.file_byte_count - 1);

//     snapu64_t buffer_address = job.buffer_address;
//     if (! mf_file_seek(mem_ddr, job.slot, file_begin_block, MF_FALSE)) return SNAP_RETC_FAILURE;
//     for (snapu64_t i_block = file_begin_block; i_block <= file_end_block; ++i_block)
//     {
//         /* mf_block_offset_t begin_offset = (i_block == file_begin_block)? file_begin_offset : 0; */
//         /* mf_block_offset_t end_offset = (i_block == file_end_block)? file_end_offset : MF_BLOCK_BYTES - 1; */
//         mf_block_offset_t begin_offset = 0;
//         if (i_block == file_begin_block)
//         {
//             begin_offset = file_begin_offset;
//         }
//         mf_block_offset_t end_offset = MF_BLOCK_BYTES - 1;
//         if (i_block == file_end_block)
//         {
//             end_offset = file_end_offset;
//         }

//         if (job.write_else_read)
//         {
//             action_file_write_block(mem_in, job.slot, buffer_address, begin_offset, end_offset);
//         }
//         else
//         {
//             action_file_read_block(mem_in, mem_out, job.slot, buffer_address, begin_offset, end_offset);
//         }

//         buffer_address += end_offset - begin_offset + 1;

//         if (i_block == file_end_block)
//         {
//             if (! mf_file_flush(mem_ddr, job.slot)) return SNAP_RETC_FAILURE;
//         }
//         else
//         {
//             if (! mf_file_next(mem_ddr, job.slot, MF_TRUE)) return SNAP_RETC_FAILURE;
//         }
//     }
//     return SNAP_RETC_SUCCESS;
// }

#define AXI_STREAM_SWITCH_MAPPING_CONFIG_OFFSET (0x40 / sizeof(uint32_t))

void switch_set_mapping(snapu32_t *switch_ctrl, snapu32_t data_in, snapu32_t data_out) {
    snapu32_t *switch_mappings = switch_ctrl + AXI_STREAM_SWITCH_MAPPING_CONFIG_OFFSET;

    switch_mappings[data_out] = data_in;
}

void switch_disable_output(snapu32_t *switch_ctrl, snapu32_t data_out) {
    snapu32_t *switch_mappings = switch_ctrl + AXI_STREAM_SWITCH_MAPPING_CONFIG_OFFSET;

    switch_mappings[data_out] = 0x80000000;
}

void switch_commit(snapu32_t *switch_ctrl) {
    switch_ctrl[0] = 0x2;
}

static snapu64_t _enable_mask = 0;

static mf_retc_t action_configure_streams(snapu32_t *switch_ctrl, snap_membus_t * mem_in, const uint64_t job_address) {
    // Everything fits into one memory line
    snap_membus_t line = mem_in[MFB_ADDRESS(job_address)];

    _enable_mask = mf_get64(line, 0);

    switch_set_mapping(switch_ctrl, mf_get32(line,  8), 0);
    switch_set_mapping(switch_ctrl, mf_get32(line, 12), 1);
    switch_set_mapping(switch_ctrl, mf_get32(line, 16), 2);
    switch_set_mapping(switch_ctrl, mf_get32(line, 20), 3);
    switch_set_mapping(switch_ctrl, mf_get32(line, 24), 4);
    switch_set_mapping(switch_ctrl, mf_get32(line, 28), 5);
    switch_set_mapping(switch_ctrl, mf_get32(line, 32), 6);
    switch_set_mapping(switch_ctrl, mf_get32(line, 36), 7);
    switch_commit(switch_ctrl);

    return SNAP_RETC_SUCCESS;
}

static mf_retc_t action_run_afus(
    snap_membus_t * mem_in,
    snap_membus_t * mem_out,
#ifdef DRAM_ENABLED
    snap_membus_t * mem_ddr_in,
    snap_membus_t * mem_ddr_out,
#endif
    mf_stream &axis_s_0,
    mf_stream &axis_s_1,
    mf_stream &axis_s_2,
    mf_stream &axis_s_3,
    mf_stream &axis_s_4,
    mf_stream &axis_s_5,
    mf_stream &axis_s_6,
    mf_stream &axis_s_7,
    mf_stream &axis_m_0,
    mf_stream &axis_m_1,
    mf_stream &axis_m_2,
    mf_stream &axis_m_3,
    mf_stream &axis_m_4,
    mf_stream &axis_m_5,
    mf_stream &axis_m_6,
    mf_stream &axis_m_7
) {
    snap_bool_t enable_0 = _enable_mask[0];
    snap_bool_t enable_1 = _enable_mask[1];
    snap_bool_t enable_2 = _enable_mask[2];
    snap_bool_t enable_3 = _enable_mask[3];
    snap_bool_t enable_4 = _enable_mask[4];
    snap_bool_t enable_5 = _enable_mask[5];
    snap_bool_t enable_6 = _enable_mask[6];
    snap_bool_t enable_7 = _enable_mask[7];
    snap_bool_t enable_8 = _enable_mask[8];
    snap_bool_t enable_9 = _enable_mask[9];

    {
#pragma HLS DATAFLOW
        // The order should only matter when executing in the test bench

        // Input AFUs
        afu_mem_read(mem_in, axis_m_0, read_mem_config, enable_0);
#ifdef DRAM_ENABLED
        afu_mem_read(mem_ddr_in, axis_m_1, read_ddr_mem_config, enable_2);
#endif

        // Processing AFUs
        afu_passthrough(axis_s_2, axis_m_2, enable_4);
        afu_change_case(axis_s_3, axis_m_3, enable_5);

        // Placeholder AFUs (to be assigned)
        afu_passthrough(axis_s_4, axis_m_4, enable_6);
        afu_passthrough(axis_s_5, axis_m_5, enable_7);
        afu_passthrough(axis_s_6, axis_m_6, enable_8);
        afu_passthrough(axis_s_7, axis_m_7, enable_9);

        // Output AFUs
        afu_mem_write(axis_s_0, mem_out, write_mem_config, enable_1);
#ifdef DRAM_ENABLED
        afu_mem_write(axis_s_1, mem_ddr_out, write_ddr_mem_config, enable_3);
#endif

        // If DRAM is disabled, we have to do something with axis_*_1 in order to set streaming direction
#ifndef DRAM_ENABLED
        afu_passthrough(axis_s_1, axis_m_1, enable_2 && enable_3);
#endif
    }
    return SNAP_RETC_SUCCESS;
}

// static void action_file_write_block(snap_membus_t * mem_in,
//                                     mf_slot_offset_t slot,
//                                     snapu64_t buffer_address,
//                                     mf_block_offset_t begin_offset,
//                                     mf_block_offset_t end_offset)
// {
//     snapu64_t end_address = buffer_address + end_offset - begin_offset;

//     snapu64_t begin_line = MFB_ADDRESS(buffer_address);
//     mfb_byteoffset_t begin_line_offset = MFB_LINE_OFFSET(buffer_address);
//     snapu64_t end_line = MFB_ADDRESS(end_address);
//     mfb_byteoffset_t end_line_offset = MFB_LINE_OFFSET(end_address);

//     mf_block_offset_t offset = begin_offset;
//     for (snapu64_t i_line = begin_line; i_line <= end_line; ++i_line)
//     {
//         snap_membus_t line = mem_in[i_line];
//         mfb_bytecount_t begin_byte = 0;
//         if (i_line == begin_line)
//         {
//             begin_byte = begin_line_offset;
//         }
//         mfb_bytecount_t end_byte = MFB_INCREMENT - 1;
//         if (i_line == end_line)
//         {
//             end_byte = end_line_offset;
//         }
//         for (mfb_bytecount_t i_byte = begin_byte; i_byte <= end_byte; ++i_byte)
//         {
//             mf_file_buffers[slot][offset++] = mf_get8(line, i_byte);
//         }
//     }
// }

// static void action_file_read_block(snap_membus_t * mem_in,
//                                    snap_membus_t * mem_out,
//                                    mf_slot_offset_t slot,
//                                    snapu64_t buffer_address,
//                                    mf_block_offset_t begin_offset,
//                                    mf_block_offset_t end_offset)
// {
//     snapu64_t end_address = buffer_address + end_offset - begin_offset;

//     snapu64_t begin_line = MFB_ADDRESS(buffer_address);
//     mfb_byteoffset_t begin_line_offset = MFB_LINE_OFFSET(buffer_address);
//     snapu64_t end_line = MFB_ADDRESS(end_address);
//     mfb_byteoffset_t end_line_offset = MFB_LINE_OFFSET(end_address);

//     mf_block_offset_t offset = begin_offset;
//     for (snapu64_t i_line = begin_line; i_line <= end_line; ++i_line)
//     {
//         snap_membus_t line = mem_in[i_line];
//         mfb_bytecount_t begin_byte = 0;
//         if (i_line == begin_line)
//         {
//             begin_byte = begin_line_offset;
//         }
//         mfb_bytecount_t end_byte = MFB_INCREMENT - 1;
//         if (i_line == end_line)
//         {
//             end_byte = end_line_offset;
//         }
//         for (mfb_bytecount_t i_byte = begin_byte; i_byte <= end_byte; ++i_byte)
//         {
//             mf_set8(line, i_byte, mf_file_buffers[slot][offset++]);
//         }
//         mem_out[i_line] = line;
//     }
// }

//-----------------------------------------------------------------------------
//--- TESTBENCH ---------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef NO_SYNTH

#include <stdio.h>
#include <endian.h>

int test_afu_mem_read() {
    static snap_membus_t src_mem[16];
    static uint8_t *src_memb = (uint8_t *) src_mem;
    static mf_stream out_stream;

    // Fill the memory with data
    for (int i = 0; i < 1024; ++i) {
        src_memb[i] = i % 256;
    }

    bool success = true;
    uint64_t offset, size;

    {
        // AFU mem read should stream entire memory line
        offset = 0, size = 64;
        afu_mem_set_config(offset, size, read_mem_config);
        afu_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mf_stream_element result;
        for (int element = 0; element < 8; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // AFU mem read should stream two entire memory lines
        offset = 0, size = 128;
        afu_mem_set_config(offset, size, read_mem_config);
        afu_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mf_stream_element result;
        for (int element = 0; element < 2 * 8; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // AFU mem read should stream a half memory line starting at offset
        offset = 16, size = 32;
        afu_mem_set_config(offset, size, read_mem_config);
        afu_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mf_stream_element result;
        for (int element = 0; element < 8 / 2; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // AFU mem read should stream one and a half memory line starting at offset
        offset = 16, size = 96;
        afu_mem_set_config(offset, size, read_mem_config);
        afu_mem_read(src_mem, out_stream, read_mem_config, true);

        mf_stream_element result;
        for (int element = 0; element < 12; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // AFU mem read should stream one and a half memory line
        offset = 0, size = 96;
        afu_mem_set_config(offset, size, read_mem_config);
        afu_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mf_stream_element result;
        for (int element = 0; element < 12; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // AFU mem read should stream a half stream word starting at offset
        offset = 16, size = 4;
        afu_mem_set_config(offset, size, read_mem_config);
        afu_mem_read(src_mem, out_stream, read_mem_config, true);

        mf_stream_element result = out_stream.read();
        success &= memcmp(&result.data, &src_memb[offset], 4) == 0;
        success &= result.strb == 0x0f;
        success &= result.last == true;

        if (!success) return -1;
    }

    return 0;
}

int test_afu_mem_write() {
    static mf_stream in_stream;
    static snap_membus_t dest_mem[16];
    static uint8_t *dest_memb = (uint8_t *) dest_mem;
    static mf_stream_element stream_elements[128];
    static uint8_t reference_data[1024];

    // Fill the stream elements with data
    for (int i = 0; i < 1024; ++i) {
        reference_data[i] = i % 256;
    }
    for (int i = 0; i < 128; ++i) {
        uint8_t *data = (uint8_t*) &stream_elements[i].data;
        for (int j = 0; j < 8; ++j) {
            data[j] = (i * 8 + j) % 256;
        }
    }

    bool success = true;
    uint64_t offset, size;

    {
        // AFU mem write should store entire memory line
        offset = 0, size = 64;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 8; ++element) {
            mf_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 7;
            in_stream.write(e);
        }

        afu_mem_set_config(offset, size, write_mem_config);
        afu_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // AFU mem write should store two entire memory lines
        offset = 0, size = 128;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 16; ++element) {
            mf_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 15;
            in_stream.write(e);
        }

        afu_mem_set_config(offset, size, write_mem_config);
        afu_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // AFU mem write should store a half memory line
        offset = 0, size = 32;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 4; ++element) {
            mf_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 3;
            in_stream.write(e);
        }

        afu_mem_set_config(offset, size, write_mem_config);
        afu_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // AFU mem write should store a half memory line at offset
        offset = 16, size = 32;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 4; ++element) {
            mf_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 3;
            in_stream.write(e);
        }

        afu_mem_set_config(offset, size, write_mem_config);
        afu_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
		// AFU mem write should store one and a half memory lines at offset
		offset = 16, size = 96;
		memset(dest_memb, 0, 1024);

		// 8 stream elements make up a memory line
		for (int element = 0; element < 12; ++element) {
			mf_stream_element e = stream_elements[element];
			e.strb = 0xff;
			e.last = element == 11;
			in_stream.write(e);
		}

		afu_mem_set_config(offset, size, write_mem_config);
		afu_mem_write(in_stream, dest_mem, write_mem_config, true);

		success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

		if (!success) return -1;
	}

    return 0;
}

int main()
{
    // Poor man's unit tests:
    if (test_afu_mem_read()) printf("Test failure\n"); else printf("Test success\n");
    if (test_afu_mem_write()) printf("Test failure\n"); else printf("Test success\n");

    static snap_membus_t host_gmem[1024];
    static snap_membus_t dram_gmem[1024];
    //static snapu32_t nvme_gmem[];
    action_reg act_reg;
    action_RO_config_reg act_config;

    // Streams
    static snapu32_t switch_ctrl[32];
    static mf_stream stream_0_2;
    static mf_stream stream_1_1; // Shortcut for disabled op
    static mf_stream stream_2_3;
    static mf_stream stream_3_4;
    static mf_stream stream_4_5;
    static mf_stream stream_5_6;
    static mf_stream stream_6_7;
    static mf_stream stream_7_0;

    mf_stream &axis_s_0 = stream_7_0;
    mf_stream &axis_s_1 = stream_1_1;
    mf_stream &axis_s_2 = stream_0_2;
    mf_stream &axis_s_3 = stream_2_3;
    mf_stream &axis_s_4 = stream_3_4;
    mf_stream &axis_s_5 = stream_4_5;
    mf_stream &axis_s_6 = stream_5_6;
    mf_stream &axis_s_7 = stream_6_7;

    mf_stream &axis_m_0 = stream_0_2;
    mf_stream &axis_m_1 = stream_1_1;
    mf_stream &axis_m_2 = stream_2_3;
    mf_stream &axis_m_3 = stream_3_4;
    mf_stream &axis_m_4 = stream_4_5;
    mf_stream &axis_m_5 = stream_5_6;
    mf_stream &axis_m_6 = stream_6_7;
    mf_stream &axis_m_7 = stream_7_0;

    // read action config:
    act_reg.Control.flags = 0x0;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);
    fprintf(stderr, "ACTION_TYPE:   %08x\nRELEASE_LEVEL: %08x\nRETC:          %04x\n",
        (unsigned int)act_config.action_type,
        (unsigned int)act_config.release_level,
        (unsigned int)act_reg.Control.Retc);


    // test action functions:

    fprintf(stderr, "// MAP slot 2 1000:7,2500:3,1700:8\n");
    uint64_t * job_mem = (uint64_t *)host_gmem;
    uint32_t * job_mem_h = (uint32_t *)host_gmem;
    uint8_t * job_mem_b = (uint8_t *)host_gmem;
    job_mem_b[0] = 2; // slot
    job_mem[1] = true; // map
    job_mem[2] = htobe64(3); // extent_count

    job_mem[8]  = htobe64(1000); // ext0.begin
    job_mem[9]  = htobe64(7);    // ext0.count
    job_mem[10] = htobe64(2500); // ext1.begin
    job_mem[11] = htobe64(3);    // ext1.count
    job_mem[12] = htobe64(1700); // ext2.begin
    job_mem[13] = htobe64(8);    // ext2.count

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_MAP;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// MAP slot 7 1200:8,1500:24\n");
    job_mem_b[0] = 7; // slot
    job_mem[1] = true; // map
    job_mem[2] = htobe64(2); // extent_count

    job_mem[8]  = htobe64(1200); // ext0.begin
    job_mem[9]  = htobe64(8);    // ext0.count
    job_mem[10] = htobe64(1500); // ext1.begin
    job_mem[11] = htobe64(24);   // ext1.count

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_MAP;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// QUERY slot 2, lblock 9\n");

    job_mem_b[0] = 2; // slot
    job_mem_b[1] = true; // query_mapping
    job_mem_b[2] = true; // query_state
    job_mem[1] = htobe64(9); // lblock

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_QUERY;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);


    fprintf(stderr, "// ACCESS slot 2, write 6K @ offset 3K\n");
    job_mem_b[0] = 2; // slot
    job_mem_b[1] = true; // write
    job_mem[1] = htobe64(128); // buffer beginning at mem[2]
    job_mem[2] = htobe64(3072); // 3K offset
    job_mem[3] = htobe64(6144); // 6K length

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
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// ACCESS slot 2, read 100 @ offset 3077\n");
    job_mem_b[0] = 2; // slot
    job_mem_b[1] = false; // read
    job_mem[1] = htobe64(7168); // buffer beginning at mem[112]
    job_mem[2] = htobe64(3077); // 3077 offset
    job_mem[3] = htobe64(100); // 100 length

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_ACCESS;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// CONFIGURE STREAMS\n");
    uint64_t enable = 0;
    enable |= (1 << 0);
    enable |= (1 << 1);
    // enable |= (1 << 2);
    // enable |= (1 << 3);
    enable |= (1 << 4);
    enable |= (1 << 5);
    enable |= (1 << 6);
    enable |= (1 << 7);
    enable |= (1 << 8);
    enable |= (1 << 9);

    job_mem[0] = htobe64(enable);

    uint32_t *master_source = job_mem_h + 2;
    master_source[0] = htobe32(7);
    master_source[1] = htobe32(0);
    master_source[2] = htobe32(1);
    master_source[3] = htobe32(2);
    master_source[4] = htobe32(3);
    master_source[5] = htobe32(4);
    master_source[6] = htobe32(5);
    master_source[7] = htobe32(6);

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_CONFIGURE_STREAMS;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);

    fprintf(stderr, "// AFU MEM SET READ BUFFER\n");
    job_mem[0] = htobe64(0x80);
    job_mem[1] = htobe64(68);
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_AFU_MEM_SET_READ_BUFFER;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);

    fprintf(stderr, "// AFU MEM SET WRITE BUFFER\n");
    job_mem[0] = htobe64(0x100);
    job_mem[1] = htobe64(126);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_AFU_MEM_SET_WRITE_BUFFER;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);

    fprintf(stderr, "// AFU CHANGE CASE SET MODE\n");
    job_mem[0] = htobe64(0);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_AFU_CHANGE_CASE_SET_MODE;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);

    fprintf(stderr, "// RUN AFUS\n");
    // Fill the memory with 'c' and 'd' characters
    memset(&job_mem_b[0x80], 0, 1024);
    memset(&job_mem_b[0x80], 'c', 64);
    memset(&job_mem_b[0x80 + 64], 'd', 64);

    // Dummy output data
    memset(&job_mem_b[0x100], 'e', 128);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MF_JOB_RUN_AFUS;
    hls_action(host_gmem, host_gmem,
#ifdef DRAM_ENABLED
        dram_gmem, dram_gmem,
#endif
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);

    *(char *)(job_mem_b + 0x100 + 128) = '\0';
    fprintf(stderr, "Result is : %s\n", (char *)(job_mem_b + 0x100));

    fprintf(stderr, "// WRITEBACK \n");
        job_mem[0] = htobe64(0);
        job_mem[1] = htobe64(0);
        job_mem[2] = htobe64(0x80);
        job_mem[3] = htobe64(1);
        act_reg.Data.job_address = 0;
        act_reg.Data.job_type = MF_JOB_WRITEBACK;
        hls_action(host_gmem, host_gmem,
    #ifdef DRAM_ENABLED
            dram_gmem, dram_gmem,
    #endif
    #ifdef NVME_ENABLED
            NULL,
    #endif
            axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, &act_reg, &act_config);


    return 0;
}

#endif /* NO_SYNTH */
