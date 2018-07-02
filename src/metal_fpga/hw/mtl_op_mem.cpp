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

void op_mem_write_impl(mtl_stream &in, snap_membus_t *dout_gmem, mtl_mem_configuration &config) {

    #pragma HLS dataflow

    hls::stream<word_stream_element> word_stream;
    hls::stream<word_stream_element> aligned_word_stream;

    {
        mtl_stream_element element;
        word_stream_element out_element = { 0, 0, 0 };
        uint8_t current_subword = 0;
        widen_stream:
        do {
            element = in.read();
            out_element.data |= (ap_uint<512>)element.data << (current_subword * sizeof(element.data) * 8);
            out_element.strb |= (ap_uint<64>)element.strb << (current_subword * sizeof(element.data));
            out_element.last = element.last;

            ++current_subword;

            if (out_element.strb == 0xffffffffffffffff) {
                word_stream.write(out_element);
                current_subword = 0;
                out_element.data = 0;
                out_element.strb = 0;
            }

            if (element.last && out_element.strb != 0) {
                word_stream.write(out_element);
            }
        } while(!element.last);
    }

    {
        word_stream_element element;
        word_stream_element current_element = { 0, 0, 0 };
        uint8_t begin_line_offset = config.offset % BPERDW;
        adjust_alignment:
        do {
            element = word_stream.read();

            current_element.data |= element.data << (begin_line_offset * 8);
            current_element.strb |= element.strb << (begin_line_offset);

            word_stream_element next_element = { 0, 0, 0 };
            next_element.data = element.data >> ((BPERDW - begin_line_offset) * 8);
            next_element.strb = element.strb >> (BPERDW - begin_line_offset);

            current_element.last = element.last && next_element.strb == 0;
            
            aligned_word_stream.write(current_element);

            if (element.last && next_element.strb != 0) {
            	next_element.last = true;
                aligned_word_stream.write(next_element);
            }

            current_element = next_element;
        } while(!element.last);
    }

    {
        const uint64_t first_word = config.offset / BPERDW;
        const uint64_t last_word = (config.offset + config.size - 1) / BPERDW;
        const uint64_t write_words = (last_word - first_word) + 1;

        word_stream_element element = { 0, 0, 0 };

        write_to_mem:
        for (int k = 0; k < write_words && !element.last; k++) {
            #pragma HLS PIPELINE
            element = aligned_word_stream.read();
            (dout_gmem + config.offset / BPERDW)[k] = element.data; // Unfortunately, we can't pass the strobe
        }
    }
}

void op_mem_write(mtl_stream &in, snap_membus_t *dout_gmem, mtl_mem_configuration &config, snap_bool_t enable) {
#pragma HLS dataflow
    if (enable) {
        op_mem_write_impl(in, dout_gmem, config);
    }
}
