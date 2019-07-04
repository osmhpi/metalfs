#include "mtl_op_mem.h"
#include "mtl_endian.h"
#include "snap_action_metal.h"
#include "axi_switch.h"

#include <snap_types.h>

#define DRAM_BASE_OFFSET 0x8000000000000000

namespace metal {
namespace fpga {

mtl_mem_configuration read_mem_config;
mtl_mem_configuration write_mem_config;

mtl_retc_t op_mem_set_config(uint64_t offset, uint64_t size, uint64_t mode, snap_bool_t read, mtl_mem_configuration &config, snapu32_t *data_preselect_switch_ctrl) {
    config.offset = offset;
    config.size = size;
    config.mode = mode;

    // Configure Data Preselector

    if (read) {
        switch (config.mode) {
            case OP_MEM_MODE_HOST:
            case OP_MEM_MODE_DRAM: {
                switch_set_mapping(data_preselect_switch_ctrl, 0, 0); // DataMover -> Metal Switch
                break;
            }
            case OP_MEM_MODE_RANDOM: {
                switch_set_mapping(data_preselect_switch_ctrl, 1, 0); // Stream Gen -> Metal Switch
                break;
            }
            default: break;
        }
    } else {
        switch (config.mode) {
            case OP_MEM_MODE_HOST:
            case OP_MEM_MODE_DRAM: {
                switch_set_mapping(data_preselect_switch_ctrl, 2, 1); // Metal Switch -> DataMover
                switch_disable_output(data_preselect_switch_ctrl, 2); // X -> Stream Sink
                break;
            }
            case OP_MEM_MODE_NULL: {
                switch_disable_output(data_preselect_switch_ctrl, 1); // X -> DataMover
                switch_set_mapping(data_preselect_switch_ctrl, 2, 2); // Metal Switch -> Stream Sink
                break;
            }
            default: break;
        }
    }

    switch_commit(data_preselect_switch_ctrl);

    return SNAP_RETC_SUCCESS;
}

void op_mem_read(
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    snapu32_t *random_ctrl,
    mtl_mem_configuration &config) {

    const uint64_t baseaddr = config.mode == OP_MEM_MODE_DRAM ? DRAM_BASE_OFFSET : 0;

    switch (config.mode) {
        case OP_MEM_MODE_HOST:
        case OP_MEM_MODE_DRAM: {
            const int N = 64; // Address width
            uint64_t bytes_read = 0;
            while (bytes_read < config.size) {
                // Stream data in block-sized chunks (64K)
                uint64_t bytes_remaining = config.size - bytes_read;
                snap_bool_t end_of_frame = bytes_remaining <= 1<<16;
                ap_uint<23> read_bytes = end_of_frame ? bytes_remaining : 1<<16;

                axi_datamover_command_t cmd = 0;
                cmd((N+31), 32) = baseaddr + config.offset + bytes_read;
                cmd[30] = end_of_frame;
                cmd[23] = 1; // AXI burst type: INCR
                cmd(22, 0) = read_bytes;
                mm2s_cmd.write(cmd);

                bytes_read += read_bytes;

                mm2s_sts.read();
            }
            break;
        }
        case OP_MEM_MODE_RANDOM: {
            random_ctrl[0x10 / sizeof(snapu32_t)] = config.size;
            random_ctrl[0] = 1;  // ap_start = true (self-clearing)
            break;
        }
        default: break;
    }
}

snap_bool_t issue_command(uint64_t bytes_remaining, uint64_t bytes_written, uint64_t addr, axi_datamover_command_stream_t &s2mm_cmd) {
    const int N = 64; // Address width
    snap_bool_t end_of_frame = bytes_remaining <= 1<<16;
    ap_uint<23> write_bytes = end_of_frame ? bytes_remaining : 1<<16;

    axi_datamover_command_t cmd = 0;
    cmd((N+31), 32) = addr + bytes_written;
    cmd[30] = end_of_frame;
    cmd[23] = 1; // AXI burst type: INCR
    cmd(22, 0) = write_bytes;
    s2mm_cmd.write(cmd);

    return end_of_frame;
}

ap_uint<32> read_status(axi_datamover_status_ibtt_stream_t &s2mm_sts) {
    axi_datamover_ibtt_status_t status = s2mm_sts.read();
    return status.data;
}

uint64_t op_mem_write(
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
    mtl_mem_configuration &config) {

    const uint64_t baseaddr = config.mode == OP_MEM_MODE_DRAM ? DRAM_BASE_OFFSET : 0;

    switch (config.mode) {
        case OP_MEM_MODE_HOST:
        case OP_MEM_MODE_DRAM: {
            uint64_t bytes_written = 0;

            for (;;) {
                // Write data in block-sized chunks (64K)
                uint64_t bytes_remaining = config.size - bytes_written;

                snap_bool_t end_of_frame = issue_command(bytes_remaining, bytes_written, baseaddr + config.offset, s2mm_cmd);

                ap_uint<32> status = read_status(s2mm_sts);
                bytes_written += status(30, 8);

                if (status[31] /* = end of packet */ || end_of_frame) {
                    return bytes_written;
                }
            }
            break;
        }
        default: break;
    }

    return 0;
}

}  // namespace fpga
}  // namespace metal
