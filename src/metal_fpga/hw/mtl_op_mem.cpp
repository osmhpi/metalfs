#include "mtl_afu_mem.h"
#include "mtl_endian.h"

#include <snap_types.h>

mtl_mem_configuration read_mem_config;
mtl_mem_configuration write_mem_config;
mtl_mem_configuration read_ddr_mem_config;
mtl_mem_configuration write_ddr_mem_config;

mtl_retc_t afu_mem_set_config(uint64_t offset, uint64_t size, mtl_mem_configuration &config) {
    config.offset = offset;
    config.size = size;
    return SNAP_RETC_SUCCESS;
}

typedef stream_element<sizeof(snap_membus_t)> word_stream_element;

void afu_mem_read_impl(snap_membus_t *din_gmem, hls::stream<word_stream_element> &mem_stream, mtl_mem_configuration &config, snap_bool_t enable) {
    if (enable) {
        snap_membus_t next_output = 0;
        uint64_t next_output_strb = 0;

        uint8_t word_offset = config.offset % BPERDW;
        uint64_t buffer_offset = 0;

        // Which is the offset of the first line we don't have to read? (Assuming config.size > 0)
        uint64_t final_offset = (MFB_ADDRESS(config.offset + config.size - 1) - MFB_ADDRESS(config.offset) + 1) * BPERDW;

        readmem_loop:
        while (true) {
            snap_membus_t output = next_output;
            uint64_t output_strb = next_output_strb;

            snap_membus_t current_word = 0;
            uint64_t current_strb = 0;

            if (buffer_offset < final_offset) {
                current_word = din_gmem[MFB_ADDRESS(config.offset + buffer_offset)];

                current_strb = 0xffffffffffffffff;
                if (buffer_offset == 0) {
                    // Mask out bytes at the beginning
                	// (lower addresses are located at the right of the snap_membus_t word)
                    current_strb >>= word_offset;
                    current_strb <<= word_offset;
                }

                uint8_t max_bytes = BPERDW - word_offset;
                if (buffer_offset + max_bytes > config.size) {
                    // Mask out bytes at the end
                    current_strb <<= max_bytes - (config.size - buffer_offset);
                    current_strb >>= max_bytes - (config.size - buffer_offset);
                }
            }

            next_output = word_offset > 0 ? current_word : (snap_membus_t)0;
            next_output >>= word_offset * 8;
            next_output_strb = word_offset > 0 ? current_strb : 0;
            next_output_strb >>= word_offset;

            current_word <<= word_offset > 0 ? (BPERDW - word_offset) * 8 : 0;
            current_strb <<= word_offset > 0 ? (BPERDW - word_offset) : 0;

            output |= current_word;
            output_strb |= current_strb;

            snap_bool_t last_element = (word_offset > 0 && next_output_strb == 0)
            		|| (word_offset == 0 && buffer_offset + BPERDW >= final_offset);

            if (output_strb) {
                word_stream_element element;
                element.data = output;
                element.strb = output_strb;
                element.last = last_element;

                mem_stream.write(element);
            }

            if (last_element) {
                break;
            }

            buffer_offset += BPERDW;
        }
    }
}


void afu_mem_read(snap_membus_t *din_gmem, mtl_stream &out, mtl_mem_configuration &config, snap_bool_t enable) {
#pragma HLS dataflow
    hls::stream<word_stream_element> mem_stream;
    afu_mem_read_impl(din_gmem, mem_stream, config, enable);
    stream_narrow(out, mem_stream, enable);
}

void afu_mem_write_impl(hls::stream<word_stream_element> &mem_stream, snap_membus_t *dout_gmem, mtl_mem_configuration &config, snap_bool_t enable) {
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

void afu_mem_write(mtl_stream &in, snap_membus_t *dout_gmem, mtl_mem_configuration &config, snap_bool_t enable) {
#pragma HLS dataflow
    hls::stream<word_stream_element> mem_stream;
    stream_widen(mem_stream, in, enable);
    afu_mem_write_impl(mem_stream, dout_gmem, config, enable);
}
