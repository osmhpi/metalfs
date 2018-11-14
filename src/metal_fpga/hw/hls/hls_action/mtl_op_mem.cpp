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

void op_mem_write_impl(mtl_stream &in, snap_membus_t *dout_gmem, mtl_mem_configuration &config) {

    #pragma HLS dataflow

    mtl_stream aligned_stream;
    hls::stream<word_stream_element> word_stream;
    hls::stream<word_stream_element> padded_word_stream;

    {
        mtl_stream_element element = { 0, 0, 0 };
        mtl_stream_element current_element = { 0, 0, 0 };
        uint8_t begin_line_offset = config.offset % 8;
        uint8_t insert_padding_elements = (config.offset % BPERDW) / 8;
        adjust_alignment:
        do {
            if (insert_padding_elements > 0) {
                --insert_padding_elements;
            } else {
                element = in.read();
            }

            current_element.data |= element.data << (begin_line_offset * 8);
            current_element.strb |= element.strb << (begin_line_offset);

            mtl_stream_element next_element = { 0, 0, 0 };
            next_element.data = element.data >> ((BPERDW - begin_line_offset) * 8);
            next_element.strb = element.strb >> (BPERDW - begin_line_offset);

            current_element.last = element.last && next_element.strb == 0;

            aligned_stream.write(current_element);

            if (element.last && next_element.strb != 0) {
                next_element.last = true;
                aligned_stream.write(next_element);
            }

            current_element = next_element;
        } while (!element.last);
    }

    {
        mtl_stream_element element;
        word_stream_element out_element = { 0, 0, 0 };
        ap_uint<3> current_subword = 0;
        widen_stream:
        do {
            element = aligned_stream.read();

            switch (current_subword) {
                case 0:
                    mtl_set64le<0 * 8>(out_element.data, element.data);
                    out_element.strb(0 * 8 + 7, 0 * 8) = element.strb;
                    break;
                case 1:
                    mtl_set64le<1 * 8>(out_element.data, element.data);
                    out_element.strb(1 * 8 + 7, 1 * 8) = element.strb;
                    break;
                case 2:
                    mtl_set64le<2 * 8>(out_element.data, element.data);
                    out_element.strb(2 * 8 + 7, 2 * 8) = element.strb;
                    break;
                case 3:
                    mtl_set64le<3 * 8>(out_element.data, element.data);
                    out_element.strb(3 * 8 + 7, 3 * 8) = element.strb;
                    break;
                case 4:
                    mtl_set64le<4 * 8>(out_element.data, element.data);
                    out_element.strb(4 * 8 + 7, 4 * 8) = element.strb;
                    break;
                case 5:
                    mtl_set64le<5 * 8>(out_element.data, element.data);
                    out_element.strb(5 * 8 + 7, 5 * 8) = element.strb;
                    break;
                case 6:
                    mtl_set64le<6 * 8>(out_element.data, element.data);
                    out_element.strb(6 * 8 + 7, 6 * 8) = element.strb;
                    break;
                case 7:
                    mtl_set64le<7 * 8>(out_element.data, element.data);
                    out_element.strb(7 * 8 + 7, 7 * 8) = element.strb;
                    break;
            }

            out_element.last = element.last;

            if (current_subword == 7 ||
                (element.last && out_element.strb != 0)
            ) {
                word_stream.write(out_element);
                current_subword = 0;
                out_element.data = 0;
                out_element.strb = 0;
            } else {
                ++current_subword;
            }
        } while(!element.last);
    }

    {
        word_stream_element element = { 0, 0, 0 };
        uint64_t element_counter = 0;
        snap_bool_t apply_padding = false;

        ap_uint<6> insert_padding_elements = (config.offset % MTL_BLOCK_BYTES) / BPERDW;

        // Insert padding elements on a 64-byte word level so that we always
        // start and stop writing on a page / block boundary
        do {
            if (insert_padding_elements > 0) {
                --insert_padding_elements;
            } else {
                element = word_stream.read();
            }

            ++element_counter;

            if (element.last && element_counter % 64) {
                apply_padding = true;
                element.last = false;
            }

            padded_word_stream.write(element);

            if (apply_padding) {
                element.data = 0;
                element.strb = 0;
                while (element_counter % 64) {
                    ++element_counter;
                    element.last = element_counter % 64 == 0;
                    padded_word_stream.write(element);
                }
            }

        } while (!element.last);
    }

    {
        const uint64_t first_block = config.offset / MTL_BLOCK_BYTES;
        const uint64_t last_block = (config.offset + config.size - 1) / MTL_BLOCK_BYTES;
        const uint64_t write_blocks = (last_block - first_block) + 1;

        word_stream_element element = { 0, 0, 0 };
        uint64_t total_blocks = 0;
        write_to_mem:
        while (!element.last && total_blocks < write_blocks) {
            // Always do burst writes of 4KB (one page / block)
            for (int k = 0; k < 64; k++) {
                #pragma HLS PIPELINE
                element = padded_word_stream.read();
                (dout_gmem + config.offset / BPERDW)[total_blocks * 64 + k] = element.data; // Unfortunately, we can't pass the strobe
            }
            ++total_blocks;
        }
    }
}

void op_mem_write_null(mtl_stream &in, mtl_mem_configuration &config) {

    #pragma HLS dataflow
    mtl_stream_element element;
    do {
        element = in.read();
    } while (!element.last);
}

void op_mem_write(
    mtl_stream &in, 
    snap_membus_t *dout_gmem, 
#ifdef DRAM_ENABLED
    snap_membus_t *dout_ddrmem,
#endif
    mtl_mem_configuration &config) {
    switch (config.mode) {
        case OP_MEM_MODE_HOST: {
            op_mem_write_impl(in, dout_gmem, config);
            break;
        }
#ifdef DRAM_ENABLED
        case OP_MEM_MODE_DRAM: {
            op_mem_write_impl(in, dout_ddrmem, config);
            break;
        }
#endif
        case OP_MEM_MODE_NULL: {
            op_mem_write_null(in, config);
            break;
        }
    }
}
