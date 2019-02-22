#include "mtl_op_mem.h"
#include "mtl_endian.h"
#include "action_metalfpga.h"
#include "axi_switch.h"

#include <snap_types.h>

#define DRAM_BASE_OFFSET 0x8000000000000000

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

typedef stream_element<sizeof(snap_membus_t)> word_stream_element;

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

void op_mem_write(
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_stream_t &s2mm_sts,
    mtl_mem_configuration &config) {

    const uint64_t baseaddr = config.mode == OP_MEM_MODE_DRAM ? DRAM_BASE_OFFSET : 0;

    switch (config.mode) {
        case OP_MEM_MODE_HOST:
        case OP_MEM_MODE_DRAM: {
            const int N = 64; // Address width
            uint64_t bytes_written = 0;
            while (bytes_written < config.size) {
                // Write data in block-sized chunks (64K)
                uint64_t bytes_remaining = config.size - bytes_written;
                snap_bool_t end_of_frame = bytes_remaining <= 1<<16;
                ap_uint<23> write_bytes = end_of_frame ? bytes_remaining : 1<<16;

                axi_datamover_command_t cmd = 0;
                cmd((N+31), 32) = baseaddr + config.offset + bytes_written;
                cmd[30] = end_of_frame;
                cmd[23] = 1; // AXI burst type: INCR
                cmd(22, 0) = write_bytes;
                s2mm_cmd.write(cmd);

                bytes_written += write_bytes;

                s2mm_sts.read();
            }
            break;
        }
        default: break;
    }
}
