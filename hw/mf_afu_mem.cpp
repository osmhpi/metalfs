#include "mf_afu_mem.h"

#include <snap_types.h>

static uint64_t _read_offset = 0;
static uint32_t _read_size = 0;

static uint64_t _write_offset = 0;
static uint32_t _write_size = 0;

mf_retc_t afu_mem_set_read_buffer(uint64_t read_offset, uint64_t read_size) {
    _read_offset = read_offset;
    _read_size = read_size;
    return SNAP_RETC_SUCCESS;
}

mf_retc_t afu_mem_set_write_buffer(uint64_t write_offset, uint64_t write_size) {
    _write_offset = write_offset;
    _write_size = write_size;
    return SNAP_RETC_SUCCESS;
}

void afu_mem_read(snap_membus_t *din_gmem, mf_stream &out) {
    typedef stream_element<sizeof(snap_membus_t)> word_stream_element;
    hls::stream<word_stream_element> mem_stream;

    #pragma HLS dataflow
    typedef char word_t[BPERDW];
    word_t current_word;
    readmem_loop:
    for (uint32_t buffer_offset = 0; buffer_offset < _read_size; buffer_offset += sizeof(current_word)) {
        uint32_t bytes_to_transfer = MIN(_read_size - buffer_offset, sizeof(current_word));

        /* Read in one memory word */
        memcpy((char*) current_word, din_gmem + _read_offset + (buffer_offset / sizeof(current_word)), sizeof(current_word));

        word_stream_element element;
        for (uint32_t i = 0; i < BPERDW; ++i) {
#pragma HLS unroll
        	element.data = (element.data << 8) | ap_uint<512>(current_word[i]);
        }
        element.last = buffer_offset + sizeof(current_word) >= _read_size;
        element.strb = 0xffffffffffffffff; // TODO: We should also process incomplete memory lines

        mem_stream.write(element);
    }

    stream_convert_width(out, mem_stream);
}

void afu_mem_write(mf_stream &in, snap_membus_t *dout_gmem) {
    typedef stream_element<sizeof(snap_membus_t)> word_stream_element;
    hls::stream<word_stream_element> mem_stream;

    #pragma HLS dataflow
    stream_convert_width(mem_stream, in);

    word_stream_element element;
    for (uint32_t mem_offset = 0; mem_offset < _write_size; mem_offset += sizeof(element.data)) {
        mem_stream.read(element);

        // We can only write entire words to memory, so the strb value does not matter here.
        typedef char word_t[BPERDW];
		word_t current_word;
		for (uint32_t i = 0; i < BPERDW; ++i) {
#pragma HLS unroll
			current_word[i] = uint8_t(element.data >> ((sizeof(element.data) - 1) * 8));
		}

        memcpy(dout_gmem + _write_offset + (mem_offset / sizeof(current_word)), (char*) current_word, sizeof(current_word));

        if (element.last) {
            break;
        }
    }
}
