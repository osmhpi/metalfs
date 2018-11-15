#include "mtl_op_mem.h"
#include "mtl_endian.h"
#include "action_metalfpga.h"

#include <snap_types.h>

mtl_mem_configuration read_mem_config;
mtl_mem_configuration write_mem_config;

mtl_retc_t op_mem_set_config(uint64_t offset, uint64_t size, uint64_t mode, mtl_mem_configuration &config) {
    config.offset = offset;
    config.size = size;
    config.mode = mode;
    return SNAP_RETC_SUCCESS;
}

typedef stream_element<sizeof(snap_membus_t)> word_stream_element;

void op_mem_read(
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    mtl_mem_configuration &config) {
    switch (config.mode) {
        case OP_MEM_MODE_HOST: {
            const int N = 64; // Address width (host mem)
            uint64_t bytes_read = 0;
            while (bytes_read < config.size) {
                uint64_t bytes_remaining = config.size - bytes_read;
                snap_bool_t end_of_frame = bytes_remaining < (1<<16);
                snapu16_t read_bytes = end_of_frame ? bytes_remaining : (1<<16 - 1);

                axi_datamover_command_t cmd = 0;
                cmd((N+31), 32) = config.offset;
                cmd[30] = end_of_frame;
                cmd(22, 0) = read_bytes;
                mm2s_cmd.write(cmd);

                bytes_read -= read_bytes;

                mm2s_sts.read();
            }
            break;
        }
#ifdef DRAM_ENABLED
        case OP_MEM_MODE_DRAM: {
            op_mem_read_impl(din_ddrmem, out, config);
            break;
        }
#endif
    }
}

void op_mem_write_null(mtl_stream &in, mtl_mem_configuration &config) {
    mtl_stream_element element;
    do {
        element = in.read();
    } while (!element.last);
}

void op_mem_write(
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_stream_t &s2mm_sts,
    mtl_mem_configuration &config) {

    switch (config.mode) {
        case OP_MEM_MODE_HOST: {
            const int N = 64; // Address width (host mem)
            uint64_t bytes_written = 0;
            while (bytes_written < config.size) {
                uint64_t bytes_remaining = config.size - bytes_written;
                snap_bool_t end_of_frame = bytes_remaining < (1<<16);
                snapu16_t write_bytes = end_of_frame ? bytes_remaining : (1<<16 - 1);

                axi_datamover_command_t cmd = 0;
                cmd((N+31), 32) = config.offset;
                cmd[30] = end_of_frame;
                cmd(22, 0) = write_bytes;
                s2mm_cmd.write(cmd);

                bytes_written -= write_bytes;

                s2mm_sts.read();
            }
            break;
        }
#ifdef DRAM_ENABLED
        case OP_MEM_MODE_DRAM: {
            op_mem_write_impl(in, dout_ddrmem, config);
            break;
        }
#endif
        // case OP_MEM_MODE_NULL: {
        //     op_mem_write_null(in, config);
        //     break;
        // }
    }
}
