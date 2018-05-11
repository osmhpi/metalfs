#include "mf_afu_mem.h"

#include <snap_types.h>

mf_mem_configuration read_mem_config;
mf_mem_configuration write_mem_config;
mf_mem_configuration read_ddr_mem_config;
mf_mem_configuration write_ddr_mem_config;

mf_retc_t afu_mem_set_config(uint64_t offset, uint64_t size, mf_mem_configuration &config) {
    config.offset = offset;
    config.size = size;
    return SNAP_RETC_SUCCESS;
}

typedef stream_element<sizeof(snap_membus_t)> word_stream_element;

void afu_mem_read_impl(snap_membus_t *din_gmem, hls::stream<word_stream_element> &mem_stream, mf_mem_configuration &config, snap_bool_t enable) {
    snap_membus_t current_word;

    /* Read in one memory word */
    memcpy(&current_word, &din_gmem[MFB_ADDRESS(config.offset)], sizeof(current_word));
    uint64_t subword_offset = config.offset % BPERDW;
    uint64_t buffer_offset = 0;

    readmem_loop:
    for (uint64_t read_memline = 0; enable; read_memline++) {

        // As we can only read entire memory lines, this is only used for setting the strb
        uint64_t bytes_to_transfer = MIN(config.size - buffer_offset, sizeof(current_word));
        snap_bool_t last = buffer_offset + sizeof(current_word) >= config.size;

        word_stream_element element;
        element.data = current_word;
        element.strb = 0xffffffffffffffff;
        uint8_t valid_bytes = subword_offset + bytes_to_transfer;
        element.strb <<= sizeof(current_word) - valid_bytes;

        if (!last || bytes_to_transfer > sizeof(current_word) - subword_offset) {
            // We still need more data, so read the next line from memory already
            memcpy(&current_word, &din_gmem[MFB_ADDRESS(config.offset) + read_memline + 1], sizeof(current_word));
        }

        if (subword_offset > 0) {
            element.data <<= subword_offset * 8;
            element.strb <<= subword_offset;

            // Because we can only use a part of the previously loaded memory line, we have to add data
            // from the next to obtain 64 bytes of payload data
            if (bytes_to_transfer > sizeof(current_word) - subword_offset) {
                word_stream_element temp_element;
                temp_element.data = current_word;
                temp_element.strb = 0xffffffffffffffff;
                valid_bytes = bytes_to_transfer > subword_offset ? subword_offset : bytes_to_transfer;
                temp_element.strb <<= sizeof(current_word) - valid_bytes;

                temp_element.data >>= (sizeof(current_word) - subword_offset) * 8;
                temp_element.strb >>= (sizeof(current_word) - subword_offset);

                element.data |= temp_element.data;
                element.strb |= temp_element.strb;
            }
        }

        element.last = last;

        uint64_t bytes_written = 0;
        for (int i = 0; i < BPERDW; ++i) {
    #pragma HLS unroll
            bytes_written += element.strb[i];
        }
        buffer_offset += bytes_written;

        mem_stream.write(element);

        if (element.last) {
            break;
        }
    }
}


void afu_mem_read(snap_membus_t *din_gmem, mf_stream &out, mf_mem_configuration &config, snap_bool_t enable) {
#pragma HLS dataflow
    hls::stream<word_stream_element> mem_stream;
    afu_mem_read_impl(din_gmem, mem_stream, config, enable);
    stream_narrow(out, mem_stream, enable);
}

uint64_t write_bytewise(snap_membus_t *mem, uint64_t offset, snap_membus_t &data, snapu64_t strb, uint64_t length) {
    uint64_t bytes_written = 0;

    char *memb = (char*)mem;

    typedef char word_t[BPERDW];
    word_t current_word;
    for (int i = 0; i < BPERDW; ++i) {
        #pragma HLS unroll
        current_word[i] = uint8_t(data >> ((sizeof(data) - 1) * 8));
        data <<= 8;
    }

    for (int mi = 0; mi < BPERDW; ++mi) {
        if (strb[BPERDW - 1 - mi] && bytes_written < length) {
            memb[offset + bytes_written] = current_word[mi];
            bytes_written += 1;
        }
    }

    return bytes_written;
}

void afu_mem_write_impl(hls::stream<word_stream_element> &mem_stream, snap_membus_t *dout_gmem, mf_mem_configuration &config, snap_bool_t enable) {
    if (enable) {
        word_stream_element rest;
        rest.data = 0;
        rest.strb = 0;

        uint16_t offset = config.offset % BPERDW;
        uint64_t bytes_written = 0;

        while (true) {
            word_stream_element input;
            word_stream_element output;

            input = mem_stream.read();

            output = input;
            output.data >>= offset * 8;
            output.strb >>= offset;

            output.data |= rest.data;
            output.strb |= rest.strb;

            rest = input;
            rest.data <<= (BPERDW - offset) * 8;
            rest.strb <<= (BPERDW - offset);

            // typedef char word_t[BPERDW];
            // word_t current_word;
            // for (int i = 0; i < BPERDW; ++i) {
            //     #pragma HLS unroll
            //     current_word[i] = uint8_t(output.data >> ((sizeof(output.data) - 1) * 8));
            //     output.data <<= 8;
            // }

            if (output.strb == 0xffffffffffffffff && config.size - bytes_written >= BPERDW) {
                // Write burst
                memcpy(dout_gmem + MFB_ADDRESS(config.offset + bytes_written), &output.data, sizeof(output.data));
                bytes_written += BPERDW;
            } else {
                bytes_written += write_bytewise(dout_gmem, config.offset + bytes_written, output.data, output.strb, config.size - bytes_written);
                // for (int mi = 0; mi < BPERDW; ++mi) {
                //     if (output.strb[BPERDW - 1 - mi] && bytes_written < config.size) {
                //         dout_gmemb[config.offset + bytes_written] = current_word[mi];
                //         bytes_written += 1;
                //     }
                // }
            }

            if (bytes_written == config.size) {
                break;
            }

            if (input.last) {
                if (rest.strb) {
                    bytes_written += write_bytewise(dout_gmem, config.offset + bytes_written, rest.data, rest.strb, config.size - bytes_written);
                    // for (int i = 0; i < BPERDW; ++i) {
                    //     #pragma HLS unroll
                    //     current_word[i] = uint8_t(output.data >> ((sizeof(output.data) - 1) * 8));
                    //     output.data <<= 8;
                    // }
                    // for (int mi = 0; mi < BPERDW; ++mi) {
                    //     if (rest.strb[BPERDW - 1 - mi] && bytes_written < config.size) {
                    //         dout_gmemb[config.offset + bytes_written] = current_word[mi];
                    //         bytes_written += 1;
                    //     }
                    // }
                }

                break;
            }
        }
    }
}

void afu_mem_write(mf_stream &in, snap_membus_t *dout_gmem, mf_mem_configuration &config, snap_bool_t enable) {
#pragma HLS dataflow
    hls::stream<word_stream_element> mem_stream;
    stream_widen(mem_stream, in, enable);
    afu_mem_write_impl(mem_stream, dout_gmem, config, enable);
}
