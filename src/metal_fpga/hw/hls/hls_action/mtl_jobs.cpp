#include "mtl_jobs.h"

#include "mtl_jobstruct.h"
#include "axi_perfmon.h"
#include "axi_switch.h"
#include "mtl_endian.h"
#include "operators.h"
#include "mtl_op_file.h"
#include "mtl_file.h"

static mtl_retc_t action_configure_streams(snapu32_t *switch_ctrl, snap_membus_t * mem_in, const uint64_t job_address) {
    // Everything fits into one memory line
    snap_membus_t line = mem_in[MFB_ADDRESS(job_address)];

    switch_set_mapping(switch_ctrl, mtl_get32<0 * sizeof(uint32_t)>(line), 0);
    switch_set_mapping(switch_ctrl, mtl_get32<1 * sizeof(uint32_t)>(line), 1);
    switch_set_mapping(switch_ctrl, mtl_get32<2 * sizeof(uint32_t)>(line), 2);
    switch_set_mapping(switch_ctrl, mtl_get32<3 * sizeof(uint32_t)>(line), 3);
    switch_set_mapping(switch_ctrl, mtl_get32<4 * sizeof(uint32_t)>(line), 4);
    switch_set_mapping(switch_ctrl, mtl_get32<5 * sizeof(uint32_t)>(line), 5);
    switch_set_mapping(switch_ctrl, mtl_get32<6 * sizeof(uint32_t)>(line), 6);
    switch_set_mapping(switch_ctrl, mtl_get32<7 * sizeof(uint32_t)>(line), 7);
    switch_commit(switch_ctrl);

    return SNAP_RETC_SUCCESS;
}


// File Map / Unmap Operation:
static mtl_retc_t action_map(snap_membus_t * mem_in, const mtl_job_map_t & job)
{
    switch (job.slot) {
        case 0: {
            mtl_extmap_load(read_extmap, job.extent_count, job.extent_address, mem_in);
            return SNAP_RETC_SUCCESS;
        }
        case 1: {
            mtl_extmap_load(write_extmap, job.extent_count, job.extent_address, mem_in);
            return SNAP_RETC_SUCCESS;
        }
        default: {
            return SNAP_RETC_FAILURE;
        }
    }
}

static mtl_retc_t action_mount(snapu32_t * nvme, const mtl_job_fileop_t & job)
{
    switch (job.slot) {
        case 0: {
            mtl_file_load_buffer(nvme, read_extmap, job.file_offset, job.dram_offset, job.length);
            return SNAP_RETC_SUCCESS;
        }
        case 1: {
            mtl_file_load_buffer(nvme, write_extmap, job.file_offset, job.dram_offset, job.length);
            return SNAP_RETC_SUCCESS;
        }
        default: {
            return SNAP_RETC_FAILURE;
        }
    }
}

static mtl_retc_t action_writeback(snapu32_t * nvme, const mtl_job_fileop_t & job)
{
    switch (job.slot) {
        case 0: {
            mtl_file_write_buffer(nvme, read_extmap, job.file_offset, job.dram_offset, job.length);
            return SNAP_RETC_SUCCESS;
        }
        case 1: {
            mtl_file_write_buffer(nvme, write_extmap, job.file_offset, job.dram_offset, job.length);
            return SNAP_RETC_SUCCESS;
        }
        default: {
            return SNAP_RETC_FAILURE;
        }
    }
}

static void configure_operators(snapu32_t *operator_ctrl, snap_membus_t * mem_in, const uint64_t job_address) {
    const snapu32_t operator_offset = 0x10000 / sizeof(uint32_t);

    snap_membus_t line = mem_in[MFB_ADDRESS(job_address)];
    snapu32_t offset  = mtl_get32< 0>(line);
    snapu32_t length  = mtl_get32< 4>(line);
    snapu32_t op      = mtl_get32< 8>(line) - 1; // Host user operator ids start at 1
    snapu32_t prepare = mtl_get32<12>(line);

    snapu8_t membus_line_offset = 4;
    line >>= membus_line_offset * sizeof(snapu32_t) * 8;

    for (snapu8_t i = 0; i < length; ++i, ++offset, membus_line_offset += sizeof(snapu32_t)) {
        #pragma HLS pipeline
        if (membus_line_offset % sizeof(snap_membus_t) == 0) {
            line = mem_in[MFB_ADDRESS(job_address) + membus_line_offset / sizeof(snap_membus_t)];
        }
        operator_ctrl[op * operator_offset + offset] = mtl_get32<0>(line);
        line >>= sizeof(snapu32_t) * 8;
    }

    operator_ctrl[op * operator_offset + 0x10 / sizeof(snapu32_t)] = prepare;
}

// Decode job_type and call appropriate action
mtl_retc_t process_action(snap_membus_t * mem_in,
                          snap_membus_t * mem_out,
#ifdef NVME_ENABLED
                          snapu32_t * nvme,
#endif
                          axi_datamover_command_stream_t &mm2s_cmd,
                          axi_datamover_status_stream_t &mm2s_sts,
                          axi_datamover_command_stream_t &s2mm_cmd,
                          axi_datamover_status_stream_t &s2mm_sts,
                          snapu32_t *metal_ctrl,
                          hls::stream<snapu8_t> &interrupt_reg,
                          action_reg * act_reg)
{
    snapu32_t *perfmon_ctrl =
        (metal_ctrl + (0x44A00000 / sizeof(uint32_t)));
    snapu32_t *data_preselect_switch_ctrl =
        (metal_ctrl + (0x44A10000 / sizeof(uint32_t)));
    snapu32_t *random_ctrl =
        (metal_ctrl + (0x44A20000 / sizeof(uint32_t)));
    snapu32_t *switch_ctrl =
        (metal_ctrl + (0x44A30000 / sizeof(uint32_t)));
    snapu32_t *operator_ctrl =
        (metal_ctrl + (0x44A40000 / sizeof(uint32_t)));

    mtl_retc_t result = SNAP_RETC_SUCCESS;

    switch (act_reg->Data.job_type) {
    case MTL_JOB_MAP:
    {
#ifdef NVME_ENABLED
        mtl_job_map_t map_job = mtl_read_job_map(mem_in, act_reg->Data.job_address);
        result = action_map(mem_in, map_job);
#else
        result = SNAP_RETC_SUCCESS;
#endif
        break;
    }
    case MTL_JOB_MOUNT:
    {
#ifdef NVME_ENABLED
        mtl_job_fileop_t mount_job = mtl_read_job_fileop(mem_in, act_reg->Data.job_address);
        result = action_mount(nvme, mount_job);
#else
        result = SNAP_RETC_SUCCESS;
#endif
        break;
    }
    case MTL_JOB_WRITEBACK:
    {
#ifdef NVME_ENABLED
        mtl_job_fileop_t writeback_job = mtl_read_job_fileop(mem_in, act_reg->Data.job_address);
        result = action_writeback(nvme, writeback_job);
#else
        result = SNAP_RETC_SUCCESS;
#endif
        break;
    }
    case MTL_JOB_CONFIGURE_STREAMS:
    {
        result = action_configure_streams(switch_ctrl, mem_in, act_reg->Data.job_address);
        break;
    }
    case MTL_JOB_RESET_PERFMON:
    {
        perfmon_reset(perfmon_ctrl);
        result = SNAP_RETC_SUCCESS;
        break;
    }
    case MTL_JOB_CONFIGURE_PERFMON:
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        result = action_configure_perfmon(perfmon_ctrl, mtl_get64<0>(line), mtl_get64<8>(line));
        break;
    }
    case MTL_JOB_READ_PERFMON_COUNTERS:
    {
        result = action_perfmon_read(mem_in + MFB_ADDRESS(act_reg->Data.job_address), perfmon_ctrl);
        break;
    }
    case MTL_JOB_RUN_OPERATORS:
    {
        snapu64_t enable_mask = act_reg->Data.direct_data[0];
        snapu64_t perfmon_en = act_reg->Data.direct_data[1];

        if (perfmon_en == 1) {
            perfmon_enable(perfmon_ctrl);
        }

        clear_operator_interrupts(interrupt_reg, metal_ctrl);
        action_run_operators(
            mem_in,
            mem_out,
            mm2s_cmd,
            mm2s_sts,
            s2mm_cmd,
            s2mm_sts,
            metal_ctrl,
            interrupt_reg,
            enable_mask
        );

    #ifndef NO_SYNTH
        if (perfmon_en == 1) {
        // perfmon_disable(perfmon_ctrl);
            perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x0;
        }
    #endif

        result = SNAP_RETC_SUCCESS;
        break;
    }
    case MTL_JOB_OP_MEM_SET_READ_BUFFER:
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        result = op_mem_set_config(mtl_get64<0>(line), mtl_get64<8>(line), mtl_get64<16>(line), true, read_mem_config, data_preselect_switch_ctrl);
        break;
    }
    case MTL_JOB_OP_MEM_SET_WRITE_BUFFER:
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        result = op_mem_set_config(mtl_get64<0>(line), mtl_get64<8>(line), mtl_get64<16>(line), false, write_mem_config, data_preselect_switch_ctrl);
        break;
    }
    case MTL_JOB_OP_CONFIGURE:
    {
        configure_operators(operator_ctrl, mem_in, act_reg->Data.job_address);
        break;
    }
    default:
        result = SNAP_RETC_FAILURE;
        break;
    }

    return result;
}
