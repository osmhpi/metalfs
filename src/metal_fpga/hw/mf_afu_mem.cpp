#include "mf_afu_mem.h"
#include "mf_endian.h"

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
//     typedef char word_t[BPERDW];
//     word_t current_word;

//     readmem_loop:
//     for (uint64_t buffer_offset = 0; enable; buffer_offset += sizeof(current_word)) {
//         /* Read in one memory word */
//         memcpy((char*) current_word, &din_gmem[MFB_ADDRESS(config.offset) + MFB_ADDRESS(buffer_offset)], sizeof(current_word));

//         // As we can only read entire memory lines, this is only used for setting the strb
//         uint64_t bytes_to_transfer = MIN(config.size - buffer_offset, sizeof(current_word));

//         word_stream_element element;
//         for (int i = 0; i < BPERDW; ++i) {
// #pragma HLS unroll
//             element.data = (element.data << 8) | ap_uint<512>(current_word[i]);
//             element.strb = (element.strb << 1) | ap_uint<64>(i < bytes_to_transfer);
//         }
//         element.last = buffer_offset + sizeof(current_word) >= config.size;

//         mem_stream.write(element);

//         if (element.last) {
//             break;
//         }
// }
    if (enable) {
        snap_membus_t next_output = 0;
        uint64_t next_output_strb = 0;

        uint8_t word_offset = config.offset % BPERDW;
        uint64_t buffer_offset = 0;

        // Which is the offset of the first line we don't have to read? (Assuming config.size > 0)
        uint64_t final_offset = (MFB_ADDRESS(config.offset + config.size) - MFB_ADDRESS(config.offset) + 1) * BPERDW;

        readmem_loop:
        while (true) {
            snap_membus_t output = next_output;
            uint64_t output_strb = next_output_strb;

            snap_membus_t current_word = 0;
            uint64_t current_strb = 0;

            if (buffer_offset < final_offset) {
                typedef char word_t[BPERDW];
                word_t current_word_buf;
                // current_word = din_gmem[MFB_ADDRESS(config.offset + buffer_offset)];
                memcpy((char*) current_word_buf, &din_gmem[MFB_ADDRESS(config.offset + buffer_offset)], sizeof(current_word_buf));
                // This reverses the byte order
                for (int i = 0; i < BPERDW; ++i) {
                #pragma HLS unroll
                    current_word = (current_word << 8) | ap_uint<512>(current_word_buf[i]);
                }

                current_strb = 0xffffffffffffffff;
                if (buffer_offset == 0) {
                    // Mask out bytes at the beginning
                    current_strb <<= word_offset;
                    current_strb >>= word_offset;
                }

                uint8_t max_bytes = buffer_offset == 0 ? BPERDW - word_offset : BPERDW;
                if (buffer_offset + BPERDW > config.size) {
                    // Mask out bytes at the end
                    current_strb >>= max_bytes - (config.size - (buffer_offset - word_offset));
                    current_strb <<= max_bytes - (config.size - (buffer_offset - word_offset));
                }
            }

            next_output = 0; // word_offset > 0 ? current_word : (snap_membus_t)0;
            next_output <<= word_offset * 8;
            next_output_strb = 0; // word_offset > 0 ? current_strb : 0;
            next_output_strb <<= word_offset;

            current_word >>= word_offset > 0 ? (BPERDW - word_offset) * 8 : 0;
            current_strb >>= word_offset > 0 ? (BPERDW - word_offset) : 0;

            output |= current_word;
            output_strb |= current_strb;

            if (output_strb) {
                word_stream_element element;
                element.data = output;
                element.strb = output_strb;
                element.last = next_output_strb == 0;

                mem_stream.write(element);
            }

            if (next_output_strb == 0) {
                break;
            }

            buffer_offset += BPERDW;
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
//     if (enable) {
//         word_stream_element element;
//         for (uint64_t mem_offset = 0; mem_offset < config.size; mem_offset += sizeof(element.data)) {
//             mem_stream.read(element);

//             // We can only write entire words to memory, so the strb value does not matter here.
//             typedef char word_t[BPERDW];
//             word_t current_word;
//             for (int i = 0; i < BPERDW; ++i) {
// #pragma HLS unroll
//                 current_word[i] = uint8_t(element.data >> ((sizeof(element.data) - 1) * 8));
//                 element.data <<= 8;
//             }

//             memcpy(dout_gmem + MFB_ADDRESS(config.offset) + MFB_ADDRESS(mem_offset), (char*) current_word, sizeof(current_word));

//             if (element.last) {
//                 break;
//             }
//         }
// }
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

            output.data = swap_membus_endianness(output.data);

            // if (output.strb == 0xffffffffffffffff && config.size - bytes_written >= BPERDW) {
                dout_gmem[MFB_ADDRESS(config.offset) + MFB_ADDRESS(bytes_written)] = output.data;
                //memcpy(dout_gmem + MFB_ADDRESS(config.offset) + MFB_ADDRESS(bytes_written), &output.data, sizeof(output.data));
                bytes_written += BPERDW;
            // } else {
            //     // snap_membus_t temp = dout_gmem[MFB_ADDRESS(config.offset + bytes_written)];
            //     // output.data |= temp;
            //     memcpy(dout_gmem + MFB_ADDRESS(config.offset + bytes_written), &output.data, sizeof(output.data));
            //     // for (int i = 0; i < BPERDW; ++i)
            //     //     bytes_written += rest.strb[i];
            //     bytes_written += BPERDW;
            //     //bytes_written += write_bytewise(dout_gmem, config.offset + bytes_written, output.data, output.strb, config.size - bytes_written);
            // }

            if (bytes_written >= config.size) {
                break;
            }

            if (input.last) {
                if (rest.strb) {
                    rest.data = swap_membus_endianness(rest.data);
                    // snap_membus_t temp = dout_gmem[MFB_ADDRESS(config.offset + bytes_written)];
                    // rest.data |= temp;
                    // memcpy(dout_gmem + MFB_ADDRESS(config.offset) + MFB_ADDRESS(bytes_written), &rest.data, sizeof(rest.data));
                    dout_gmem[MFB_ADDRESS(config.offset) + MFB_ADDRESS(bytes_written)] = rest.data;
                    // for (int i = 0; i < BPERDW; ++i)
                    //     bytes_written += rest.strb[i];
                    bytes_written += BPERDW;
                    //bytes_written += write_bytewise(dout_gmem, config.offset + bytes_written, rest.data, rest.strb, config.size - bytes_written);
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
