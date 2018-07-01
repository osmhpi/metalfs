#include "mtl_op_mem.h"
#include "mtl_endian.h"

#include <snap_types.h>

mtl_mem_configuration read_mem_config;
mtl_mem_configuration write_mem_config;
mtl_mem_configuration read_ddr_mem_config;
mtl_mem_configuration write_ddr_mem_config;

mtl_retc_t op_mem_set_config(uint64_t offset, uint64_t size, mtl_mem_configuration &config) {
    config.offset = offset;
    config.size = size;
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
        read_memory:
        for (int k = 0; k < read_words; k++) {
            #pragma HLS PIPELINE
            word_stream_element element;
            element.data = (din_gmem + config.offset / BPERDW)[k];
            element.strb = 0xffffffffffffffff;
            element.last = k == read_words - 1;
            word_stream.write(element);
        }
    }

    {
        uint8_t begin_line_offset = config.offset % BPERDW;
        uint8_t end_line_offset = config.offset + config.size % BPERDW;
        uint64_t first_line_strb = 0xffffffffffffffff << begin_line_offset;
        uint64_t last_line_strb = 0xffffffffffffffff >> (BPERDW - end_line_offset);

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

void op_mem_read(snap_membus_t *din_gmem, mtl_stream &out, mtl_mem_configuration &config, snap_bool_t enable) {
#pragma HLS dataflow
    if (enable) {
        op_mem_read_impl(din_gmem, out, config);
    }
}

void op_mem_write_impl(hls::stream<word_stream_element> &mem_stream, snap_membus_t *dout_gmem, mtl_mem_configuration &config, snap_bool_t enable) {
    if (enable) {
        word_stream_element rest;
        rest.data = 0;
        rest.strb = 0;

        uint16_t offset = config.offset % BPERDW;
        uint64_t current_memory_line = 0;
        uint64_t bytes_written = 0;

        while (true) {
            word_stream_element input;
            word_stream_element output;

            input = mem_stream.read();

            output = input;
            output.data <<= offset * 8;
            output.strb <<= offset;

            output.data |= rest.data;
            output.strb |= rest.strb;

            rest = input;
            rest.data >>= (BPERDW - offset) * 8;
            rest.strb >>= (BPERDW - offset);

            dout_gmem[MFB_ADDRESS(config.offset) + current_memory_line] = output.data;
            bytes_written += current_memory_line == 0 ? BPERDW - offset : BPERDW;
            current_memory_line++;

            if (bytes_written >= config.size) {
                break;
            }

            if (input.last) {
                if (rest.strb) {
                    dout_gmem[MFB_ADDRESS(config.offset) + current_memory_line] = rest.data;
                }

                break;
            }
        }
    }
}

void op_mem_write(mtl_stream &in, snap_membus_t *dout_gmem, mtl_mem_configuration &config, snap_bool_t enable) {
#pragma HLS dataflow
    hls::stream<word_stream_element> mem_stream;
    stream_widen(mem_stream, in, enable);
    op_mem_write_impl(mem_stream, dout_gmem, config, enable);
}
