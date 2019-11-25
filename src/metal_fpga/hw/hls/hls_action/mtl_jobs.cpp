#include "mtl_jobs.h"

#include "axi_perfmon.h"
#include "axi_switch.h"
#include "mtl_endian.h"
#include "operators.h"

namespace metal {
namespace fpga {

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
static mtl_retc_t action_map(snap_membus_t * mem_in, const uint64_t job_address)
{
    snap_membus_t line = mem_in[MFB_ADDRESS(job_address)];
    auto slot = reinterpret_cast<ExtmapSlot>(mtl_get64<0>(line));
    auto extent_address = job_address + MFB_INCREMENT;

    switch (slot) {
        case ExtmapSlot::CardDRAMRead: {
            mtl_extmap_load(dram_read_extmap, extent_address, mem_in);
            return SNAP_RETC_SUCCESS;
        }
        case ExtmapSlot::CardDRAMWrite: {
            mtl_extmap_load(dram_write_extmap, extent_address, mem_in);
            return SNAP_RETC_SUCCESS;
        }
#ifdef NVME_ENABLED
        case ExtmapSlot::NVMeRead: {
            mtl_extmap_load(nvme_read_extmap, extent_address, mem_in);
            return SNAP_RETC_SUCCESS;
        }
        case ExtmapSlot::NVMeWrite: {
            mtl_extmap_load(nvme_write_extmap, extent_address, mem_in);
            return SNAP_RETC_SUCCESS;
        }
#endif
        default: {
            return SNAP_RETC_FAILURE;
        }
    }
}

static void configure_operators(snapu32_t *operator_ctrl, snap_membus_t * mem_in, const uint64_t job_address) {
    snap_membus_t line = mem_in[MFB_ADDRESS(job_address)];
    snapu32_t offset  = mtl_get32< 0>(line);
    snapu32_t length  = mtl_get32< 4>(line);
    snapu32_t op      = mtl_get32< 8>(line) - 1; // Host user operator ids start at 1
    snapu32_t prepare = mtl_get32<12>(line);

    snapu8_t membus_line_offset = 4;
    line >>= membus_line_offset * sizeof(snapu32_t) * 8;

    for (auto i = 0; i < length; ++i, ++offset, membus_line_offset += sizeof(snapu32_t)) {
        #pragma HLS pipeline
        if (membus_line_offset % sizeof(snap_membus_t) == 0) {
            line = mem_in[MFB_ADDRESS(job_address) + membus_line_offset / sizeof(snap_membus_t)];
        }
        operator_ctrl[op * OperatorOffset + offset] = line(31, 0);
        line >>= sizeof(snapu32_t) * 8;
    }

    operator_ctrl[op * OperatorOffset + 0x10 / sizeof(snapu32_t)] = prepare;
}

// Decode job_type and call appropriate action
mtl_retc_t process_action(snap_membus_t * mem_in,
                          snap_membus_t * mem_out,
#ifdef NVME_ENABLED
                          NVMeCommandStream &nvme_read_cmd,
                          NVMeResponseStream &nvme_read_resp,
                          NVMeCommandStream &nvme_write_cmd,
                          NVMeResponseStream &nvme_write_resp,
#endif
                          axi_datamover_command_stream_t &mm2s_cmd,
                          axi_datamover_status_stream_t &mm2s_sts,
                          axi_datamover_command_stream_t &s2mm_cmd,
                          axi_datamover_status_ibtt_stream_t &s2mm_sts,
                          snapu32_t *metal_ctrl,
                          hls::stream<snapu8_t> &interrupt_reg,
                          action_reg * act_reg)
{
    snapu32_t *perfmon_ctrl =
        (metal_ctrl + PerfmonBaseAddr);
    snapu32_t *data_preselect_switch_ctrl =
        (metal_ctrl + DataPreselectSwitchBaseAddr);
    snapu32_t *random_ctrl =
        (metal_ctrl + RandomBaseAddr);
    snapu32_t *image_info_ctrl =
        (metal_ctrl + ImageInfoBaseAddr);
    snapu32_t *switch_ctrl =
        (metal_ctrl + SwitchBaseAddr);
    snapu32_t *operator_ctrl =
        (metal_ctrl + OperatorBaseAddr);

    mtl_retc_t result = SNAP_RETC_SUCCESS;

    switch (act_reg->Data.job_type) {
    case JobType::ReadImageInfo:
    {
        uint32_t info_len = image_info_ctrl[0x0 / sizeof(uint32_t)];
        uint32_t read_len = info_len > 4096 ? 4096 : info_len;

        snap_membus_t current_line = 0;
        int i = 0;
        for (; i < read_len; i += sizeof(uint32_t)) {
            #pragma HLS PIPELINE
            int lineoffset = i % sizeof(snap_membus_t);
            current_line(31 + 8*lineoffset, 8*lineoffset) = image_info_ctrl[(4096 + i) / sizeof(uint32_t)];

            if (lineoffset == sizeof(snap_membus_t) - sizeof(uint32_t)) {
                mem_in[MFB_ADDRESS(act_reg->Data.job_address) + (i / sizeof(snap_membus_t))] = current_line;
                current_line = 0;
            }
        }

        if (i % sizeof(snap_membus_t)) {
            mem_in[MFB_ADDRESS(act_reg->Data.job_address) + (i / sizeof(snap_membus_t))] = current_line;
        }

        act_reg->Data.direct_data[2] = read_len;
        result = SNAP_RETC_SUCCESS;
        break;
    }
    case JobType::Map:
    {
#ifdef NVME_ENABLED
        result = action_map(mem_in, act_reg->Data.job_address);
#else
        result = SNAP_RETC_SUCCESS;
#endif
        break;
    }
    case JobType::ConfigureStreams:
    {
        result = action_configure_streams(switch_ctrl, mem_in, act_reg->Data.job_address);
        break;
    }
    case JobType::ResetPerfmon:
    {
        perfmon_reset(perfmon_ctrl);
        result = SNAP_RETC_SUCCESS;
        break;
    }
    case JobType::ConfigurePerfmon:
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        result = action_configure_perfmon(perfmon_ctrl, mtl_get64<0>(line), mtl_get64<8>(line));
        break;
    }
    case JobType::ReadPerfmonCounters:
    {
        result = action_perfmon_read(mem_in + MFB_ADDRESS(act_reg->Data.job_address), perfmon_ctrl);
        break;
    }
    case JobType::RunOperators:
    {
        op_mem_set_config(act_reg->Data.source, true, read_mem_config, data_preselect_switch_ctrl);
        op_mem_set_config(act_reg->Data.destination, false, write_mem_config, data_preselect_switch_ctrl);

        snapu64_t enable_mask = act_reg->Data.direct_data[0];
        snapu64_t perfmon_en = act_reg->Data.direct_data[1];

        clear_operator_interrupts(interrupt_reg, metal_ctrl);

        #ifdef NVME_ENABLED
        if (write_mem_config.type == AddressType::NVMe) {
            preload_nvme_blocks(write_mem_config, dram_write_extmap, nvme_write_extmap, nvme_read_cmd, nvme_read_resp);
        }
        #endif

        if (perfmon_en == 1) {
            perfmon_enable(perfmon_ctrl);
        }

        snapu64_t bytes_written;
        action_run_operators(
            mem_in,
            mem_out,
            mm2s_cmd,
            mm2s_sts,
            s2mm_cmd,
            s2mm_sts,
            metal_ctrl,
            interrupt_reg,
#ifdef NVME_ENABLED
            nvme_read_cmd,
            nvme_read_resp,
            nvme_write_cmd,
            nvme_write_resp,
#endif
            enable_mask,
            &bytes_written
        );

        act_reg->Data.direct_data[2] = bytes_written;

    #ifndef NO_SYNTH
        if (perfmon_en == 1) {
            perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x0;
        }
    #endif

        result = SNAP_RETC_SUCCESS;
        break;
    }
    case JobType::ConfigureOperator:
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

}  // namespace fpga
}  // namespace metal
