#include "mf_afu_mem.h"

#include <snap_types.h>

static uint64_t _read_offset = 0;
static uint64_t _read_size = 0;

static uint64_t _write_offset = 0;
static uint64_t _write_size = 0;

mf_retc_t afu_mem_set_read_buffer(uint64_t read_offset, uint64_t read_size) {
    _read_offset = MFB_ADDRESS(read_offset);
    _read_size = read_size;
    return SNAP_RETC_SUCCESS;
}

mf_retc_t afu_mem_set_write_buffer(uint64_t write_offset, uint64_t write_size) {
    _write_offset = MFB_ADDRESS(write_offset);
    _write_size = write_size;
    return SNAP_RETC_SUCCESS;
}

typedef stream_element<sizeof(snap_membus_t)> word_stream_element;

void afu_mem_read_impl(snap_membus_t *din_gmem, hls::stream<word_stream_element> &mem_stream, snap_bool_t enable) {
    typedef char word_t[BPERDW];
    word_t current_word;

    readmem_loop:
    for (uint64_t buffer_offset = 0; enable; buffer_offset += sizeof(current_word)) {
        /* Read in one memory word */
        memcpy((char*) current_word, &din_gmem[_read_offset + MFB_ADDRESS(buffer_offset)], sizeof(current_word));

        // As we can only read entire memory lines, this is only used for setting the strb
        uint64_t bytes_to_transfer = MIN(_read_size - buffer_offset, sizeof(current_word));

        word_stream_element element;
        for (int i = 0; i < BPERDW; ++i) {
#pragma HLS unroll
            element.data = (element.data << 8) | ap_uint<512>(current_word[i]);
            element.strb = (element.strb << 1) | ap_uint<64>(i < bytes_to_transfer);
        }
        element.last = buffer_offset + sizeof(current_word) >= _read_size;

        mem_stream.write(element);

        if (element.last) {
            break;
        }
    }
}


void afu_mem_read(snap_membus_t *din_gmem, mf_stream &out, snap_bool_t enable) {
#pragma HLS dataflow
    hls::stream<word_stream_element> mem_stream;
    afu_mem_read_impl(din_gmem, mem_stream, enable);
    stream_narrow(out, mem_stream, enable);
}

void afu_mem_write_impl(hls::stream<word_stream_element> &mem_stream, snap_membus_t *dout_gmem, snap_bool_t enable) {
    if (enable) {
        word_stream_element element;
        for (uint64_t mem_offset = 0; mem_offset < _write_size; mem_offset += sizeof(element.data)) {
            mem_stream.read(element);

            // We can only write entire words to memory, so the strb value does not matter here.
            typedef char word_t[BPERDW];
            word_t current_word;
            for (int i = 0; i < BPERDW; ++i) {
#pragma HLS unroll
                current_word[i] = uint8_t(element.data >> ((sizeof(element.data) - 1) * 8));
                element.data <<= 8;
            }

            memcpy(dout_gmem + _write_offset + MFB_ADDRESS(mem_offset), (char*) current_word, sizeof(current_word));

            if (element.last) {
                break;
            }
        }
    }
}

void afu_mem_write(mf_stream &in, snap_membus_t *dout_gmem, snap_bool_t enable) {
#pragma HLS dataflow
    hls::stream<word_stream_element> mem_stream;
    stream_widen(mem_stream, in, enable);
    afu_mem_write_impl(mem_stream, dout_gmem, enable);
}
