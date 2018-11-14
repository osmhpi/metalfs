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

void op_mem_read_impl(snap_membus_t *din_gmem, mtl_stream &out, mtl_mem_configuration &config) {
#pragma HLS dataflow
    hls::stream<word_stream_element> word_stream;
    hls::stream<word_stream_element> word_stream_with_strb;
    mtl_stream mtl_word_stream;
    mtl_stream mtl_word_stream_aligned;

    {
        const uint64_t first_word = config.offset / BPERDW;
        const uint64_t last_word = (config.offset + config.size - 1) / BPERDW;
        const uint64_t read_words = (last_word - first_word) + 1;
        uint64_t total_read_words = 0;

        while (total_read_words < read_words) {
            uint64_t read_burst = (read_words - total_read_words) > 64 ? 64 : (read_words - total_read_words);
            read_memory:
            for (int k = 0; k < read_burst; k++) {
                #pragma HLS PIPELINE
                word_stream_element element;
                element.data = (din_gmem + config.offset / BPERDW)[total_read_words + k];
                element.strb = 0xffffffffffffffff;
                element.last = total_read_words + k == read_words - 1;
                word_stream.write(element);
            }
            total_read_words += read_burst;
        }
    }

    {
        uint8_t begin_line_offset = config.offset % BPERDW;
        uint8_t end_line_offset = (config.offset + config.size) % BPERDW;
        uint64_t first_line_strb = 0xffffffffffffffff << begin_line_offset;
        uint64_t last_line_strb = end_line_offset ? 0xffffffffffffffff >> (BPERDW - end_line_offset) : 0xffffffffffffffff;

        uint64_t word_count = 0;

        word_stream_element element;
        populate_strobe:
        do {
            element = word_stream.read();

            if (word_count == 0) {
                element.strb &= first_line_strb;
            }
            if (element.last) {
                element.strb &= last_line_strb;
            }

            word_stream_with_strb.write(element);

            ++word_count;
        } while(!element.last);
    }

    {
        word_stream_element element;
        narrow_stream:
        do {
            element = word_stream_with_strb.read();

            for (int i = 0; i < 8; ++i) {
                #pragma HLS unroll
                mtl_stream_element mtl_element;
                mtl_element.data = (element.data >> (i * 8 * 8));
                mtl_element.strb = (element.strb >> (i * 8));
                mtl_element.last = element.last && i == 7;

                mtl_word_stream.write(mtl_element);
            }
        } while(!element.last);
    }

    {
        mtl_stream_element current_element, previous_element;
        snap_bool_t previous_element_valid = false;
        uint8_t begin_element_offset = config.offset % sizeof(current_element.data);

        remove_empty_leading_bytes:
        do {
            current_element = mtl_word_stream.read();

            previous_element.data |= current_element.data << (8 * (8 - begin_element_offset));
            previous_element.strb |= current_element.strb << (8 - begin_element_offset);

            if (previous_element_valid)
                mtl_word_stream_aligned.write(previous_element);

            previous_element.data = current_element.data >> (8 * begin_element_offset);
            previous_element.strb = current_element.strb >> (begin_element_offset);
            previous_element.last = false;
            previous_element_valid = true;

            if (current_element.last && previous_element_valid) {
                previous_element.last = true;
                mtl_word_stream_aligned.write(previous_element);
            }
        } while(!current_element.last);
    }

    {
        mtl_stream_element current_element, previous_element;
        snap_bool_t previous_element_valid = false;

        discard_empty_elements:
        do {
            current_element = mtl_word_stream_aligned.read();

            if (current_element.strb != 0) {
                if (previous_element_valid)
                    out.write(previous_element);
                previous_element = current_element;
                previous_element_valid = true;
            }

            if (current_element.last && previous_element_valid) {
                previous_element.last = true;
                out.write(previous_element);
            }
        } while(!current_element.last);
    }
}

void op_mem_read(
    snap_membus_t *din_gmem,
#ifdef DRAM_ENABLED
    snap_membus_t *din_ddrmem,
#endif
    mtl_stream &out,
    mtl_mem_configuration &config) {
    switch (config.mode) {
        case OP_MEM_MODE_HOST: {
            op_mem_read_impl(din_gmem, out, config);
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
