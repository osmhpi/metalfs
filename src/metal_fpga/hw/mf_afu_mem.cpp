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
    typedef char word_t[BPERDW];
    word_t current_word;

    /* Read in one memory word */
    memcpy((char*) current_word, &din_gmem[MFB_ADDRESS(config.offset)], sizeof(current_word));
    uint64_t subword_offset = config.offset % BPERDW;
    uint64_t buffer_offset = 0;

    readmem_loop:
    for (uint64_t read_memline = 0; enable; read_memline++) {

        // As we can only read entire memory lines, this is only used for setting the strb
        uint64_t bytes_to_transfer = MIN(config.size - buffer_offset, sizeof(current_word));
        snap_bool_t last = buffer_offset + sizeof(current_word) >= config.size;

        word_stream_element element;
        for (int i = 0; i < BPERDW; ++i) {
#pragma HLS unroll
            element.data = (element.data << 8) | ap_uint<512>(current_word[i]);
            element.strb = (element.strb << 1) | ap_uint<64>(i < subword_offset + bytes_to_transfer);
        }

        if (!last || bytes_to_transfer > sizeof(current_word) - subword_offset) {
            // We still need more data, so read the next line from memory already
            memcpy((char*) current_word, &din_gmem[MFB_ADDRESS(config.offset) + read_memline + 1], sizeof(current_word));
        }

        if (subword_offset > 0) {
            element.data <<= subword_offset * 8;
            element.strb <<= subword_offset;

            // Because we can only use a part of the previously loaded memory line, we have to add data
            // from the next to obtain 64 bytes of payload data
            if (bytes_to_transfer > sizeof(current_word) - subword_offset) {
                word_stream_element temp_element;
                for (int i = 0; i < BPERDW; ++i) {
        #pragma HLS unroll
                    temp_element.data = (temp_element.data << 8) | ap_uint<512>(current_word[i]);
                    temp_element.strb = (temp_element.strb << 1) | ap_uint<64>(i < bytes_to_transfer - sizeof(current_word) - subword_offset);
                }

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
        	if (element.strb[i]) bytes_written++;
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

void afu_mem_write_impl(hls::stream<word_stream_element> &mem_stream, snap_membus_t *dout_gmem, mf_mem_configuration &config, snap_bool_t enable) {
    if (enable) {
        word_stream_element rest;
        rest.data = 0;
        rest.strb = 0;

        uint16_t offset = config.offset % BPERDW;
        uint64_t bytes_written = 0;

        char *dout_gmemb = (char*)dout_gmem;

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

            typedef char word_t[BPERDW];
            word_t current_word;
            for (int i = 0; i < BPERDW; ++i) {
                #pragma HLS unroll
                current_word[i] = uint8_t(output.data >> ((sizeof(output.data) - 1) * 8));
                output.data <<= 8;
            }

            if (output.strb == 0xffffffffffffffff && config.size - bytes_written >= BPERDW) {
                // Write burst
                memcpy(dout_gmem + MFB_ADDRESS(config.offset + bytes_written), (char*) current_word, sizeof(current_word));
                bytes_written += BPERDW;
            } else {
                for (int mi = 0; mi < BPERDW; ++mi) {
                    if (output.strb[BPERDW - 1 - mi] && bytes_written < config.size) {
                        dout_gmemb[config.offset + bytes_written] = current_word[mi];
                        bytes_written += 1;
                    }
                }
            }

            if (bytes_written == config.size) {
                break;
            }

            if (input.last) {
                if (rest.strb) {
                    for (int i = 0; i < BPERDW; ++i) {
                        #pragma HLS unroll
                        current_word[i] = uint8_t(output.data >> ((sizeof(output.data) - 1) * 8));
                        output.data <<= 8;
                    }
                    for (int mi = 0; mi < BPERDW; ++mi) {
                        if (rest.strb[BPERDW - 1 - mi] && bytes_written < config.size) {
                            dout_gmemb[config.offset + bytes_written] = current_word[mi];
                            bytes_written += 1;
                        }
                    }
                }

                break;
            }
        }
    }


    // if (enable) {
    //     word_stream_element input_element;
    //     word_stream_element write_element;

    //     uint64_t subword_offset = config.offset % BPERDW;
    //     uint64_t write_rest_bytes = BPERDW - subword_offset;

    //     // First, read some input that we can write
    //     mem_stream.read(input_element);

    //     for (uint64_t mem_offset = 0; mem_offset < config.size; mem_offset += sizeof(write_element.data)) {
    //         // Write the next element to memory

    //         // Move data to the right position
    //         word_stream_element temp_element;
    //         for (int i = 0; i < BPERDW; ++i) {
    // #pragma HLS unroll
    //             temp_element.data = i < write_rest_bytes ? (temp_element.data << 8) | ap_uint<512>(uint8_t(input_element.data >> ((sizeof(input_element.data) - 1) * 8))) : temp_element.data;
    //             temp_element.strb = i < write_rest_bytes ? (temp_element.strb << 1) | ap_uint<64>(uint8_t(input_element.data >> ((sizeof(input_element.strb) - 1)))) : temp_element.strb;
    //         }

    //         write_element.data |= temp_element.data;
    //         write_element.strb |= temp_element.strb;

    //     }
    // }





    // if (enable) {
    //     uint64_t subword_offset = config.offset % BPERDW;
    //     word_stream_element element;
    //     mem_stream.read(element);

    //     word_stream_element temp_element;
    //     for (uint64_t mem_offset = 0; mem_offset < config.size; mem_offset += sizeof(element.data)) {
    //         word_stream_element temp_temp_element;

    //         for (int i = 0; i < BPERDW; ++i) {
    // #pragma HLS unroll
    //             temp_temp_element.data = i < subword_offset ? (temp_temp_element.data << 8) | ap_uint<512>(current_word[i]) : temp_temp_element.data;
    //             temp_temp_element.strb = i < subword_offset ? (temp_temp_element.strb << 1) | ap_uint<64>(i < bytes_to_transfer - sizeof(current_word) - subword_offset) : temp_temp_element.strb;
    //         }

    //         temp_element.data |= temp_temp_element.data;
    //         temp_element.strb |= temp_temp_element.strb;

    //         if (temp_element.strb == 0xff) {
    //             // Write burst

    //             typedef char word_t[BPERDW];
    //             word_t current_word;
    //             for (int i = 0; i < BPERDW; ++i) {
    //                 #pragma HLS unroll
    //                 current_word[i] = uint8_t(element.data >> ((sizeof(element.data) - 1) * 8));
    //                 element.data <<= 8;
    //             }

    //             memcpy(dout_gmem + MFB_ADDRESS(config.offset + mem_offset), (char*) current_word, sizeof(current_word));
    //         } else {
    //             char *dout_gmemb = (char*)dout_gmem;
    //             for (uint16_t mi = 0; mi < write_length; ++mi)
    //                 dout_gmemb[config.offset + mem_offset] = current_word[mi];
    //         }

    //         if (element.last) {
    //             break;
    //         }

    //         if (subword_offset > 0) {
    //             for (int i = 0; i < BPERDW; ++i) {
    //     #pragma HLS unroll
    //                 temp_element.data = i < subword_offset ? (temp_element.data << 8) | ap_uint<512>(current_word[i]) : temp_element.data;
    //                 temp_element.strb = i < subword_offset ? (temp_element.strb << 1) | ap_uint<64>(i < bytes_to_transfer - sizeof(current_word) - subword_offset) : temp_element.strb;
    //             }
    //         }
    //     }
    // }
}

void afu_mem_write(mf_stream &in, snap_membus_t *dout_gmem, mf_mem_configuration &config, snap_bool_t enable) {
#pragma HLS dataflow
    hls::stream<word_stream_element> mem_stream;
    stream_widen(mem_stream, in, enable);
    afu_mem_write_impl(mem_stream, dout_gmem, config, enable);
}
