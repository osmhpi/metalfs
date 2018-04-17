#include "mf_afu_mem.h"

#include <snap_types.h>

mf_mem_configuration read_mem_config;
mf_mem_configuration write_mem_config;
mf_mem_configuration read_ddr_mem_config;
mf_mem_configuration write_ddr_mem_config;

mf_retc_t afu_mem_set_config(uint64_t offset, uint64_t size, mf_mem_configuration &config) {
    config.offset = MFB_ADDRESS(offset);
    config.size = size;
    return SNAP_RETC_SUCCESS;
}

typedef stream_element<sizeof(snap_membus_t)> word_stream_element;

void afu_mem_read_impl(snap_membus_t *din_gmem, hls::stream<word_stream_element> &mem_stream, mf_mem_configuration &config, snap_bool_t enable) {
    typedef char word_t[BPERDW];
    word_t current_word;

    readmem_loop:
    for (uint64_t buffer_offset = 0; enable; buffer_offset += sizeof(current_word)) {
        /* Read in one memory word */
        memcpy((char*) current_word, &din_gmem[config.offset + MFB_ADDRESS(buffer_offset)], sizeof(current_word));

        // As we can only read entire memory lines, this is only used for setting the strb
        uint64_t bytes_to_transfer = MIN(config.size - buffer_offset, sizeof(current_word));

        word_stream_element element;
        for (int i = 0; i < BPERDW; ++i) {
#pragma HLS unroll
            element.data = (element.data << 8) | ap_uint<512>(current_word[i]);
            element.strb = (element.strb << 1) | ap_uint<64>(i < bytes_to_transfer);
        }
        element.last = buffer_offset + sizeof(current_word) >= config.size;

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
        word_stream_element element;
        for (uint64_t mem_offset = 0; mem_offset < config.size; mem_offset += sizeof(element.data)) {
            mem_stream.read(element);

            // We can only write entire words to memory, so the strb value does not matter here.
            typedef char word_t[BPERDW];
            word_t current_word;
            for (int i = 0; i < BPERDW; ++i) {
#pragma HLS unroll
                current_word[i] = uint8_t(element.data >> ((sizeof(element.data) - 1) * 8));
                element.data <<= 8;
            }

            memcpy(dout_gmem + config.offset + MFB_ADDRESS(mem_offset), (char*) current_word, sizeof(current_word));

            if (element.last) {
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

void afu_mem_readwrite(
    mf_stream &in,
    mf_stream &out,
    snap_membus_t *mem,
    mf_mem_configuration &read_config,
    mf_mem_configuration &write_config,
    snap_bool_t read_enable,
    snap_bool_t write_enable) {

    if (read_enable && !write_enable) {
        afu_mem_read(mem, out, read_config, true);
    } else if (!read_enable && write_enable) {
        afu_mem_write(in, mem, write_config, true);
    } else if (read_enable && write_enable) {
        for (;;) {
            mf_stream_element element = in.read();
            if (element.last)
                break;
        }
    }
}
