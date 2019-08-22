#pragma once

#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <hls_snap.H>

#ifndef STREAM_BYTES
#define STREAM_BYTES 8
#endif

struct byte_stream_element {
    snapu8_t data;
    snap_bool_t last;
};
typedef hls::stream<byte_stream_element> mtl_byte_stream;

typedef ap_uint<8 * STREAM_BYTES> mtl_stream_data;
typedef ap_uint<STREAM_BYTES> mtl_stream_keep;
typedef struct {
    mtl_stream_data data;
    mtl_stream_keep keep;
    snap_bool_t last;
} mtl_stream_element;
typedef hls::stream<mtl_stream_element> mtl_stream;

template<int bytes>
void insert_padding(mtl_stream &in, mtl_stream &out) {
  mtl_stream_element element = { 0, 0, 0 };
  mtl_stream_element current_element = { 0, 0, 0 };
  mtl_stream_element next_element = { 0, 0, 0 };

  insert_padding:
  do {
#pragma HLS PIPELINE
    element = in.read();

    const auto word_bytes = sizeof(current_element.data);

    current_element.data((word_bytes * 8) - 1, bytes * 8) = element.data(((word_bytes - bytes) * 8) - 1, 0);
    current_element.keep((word_bytes    ) - 1, bytes    ) = element.keep(((word_bytes - bytes)    ) - 1, 0);

    next_element.data((bytes * 8) - 1, 0) = element.data(((word_bytes) * 8) - 1, (word_bytes - bytes) * 8);
    next_element.keep((bytes    ) - 1, 0) = element.keep(((word_bytes)    ) - 1, (word_bytes - bytes)    );

    current_element.last = element.last && next_element.keep == 0;

    out.write(current_element);

    current_element = next_element;
  } while (!element.last);

  if (next_element.keep != 0) {
    next_element.last = true;
    out.write(next_element);
  }
}

template<int bytes>
void remove_padding(mtl_stream & in, mtl_stream &out) {
  mtl_stream_element current_element, previous_element, out_element;
  snap_bool_t previous_element_valid = false;

  remove_padding:
  do {
#pragma HLS PIPELINE
    current_element = in.read();

    const auto word_bytes = sizeof(current_element.data);

    out_element.data(((word_bytes - bytes) * 8) - 1, 0) = previous_element.data((word_bytes * 8) - 1, bytes * 8);
    out_element.keep(((word_bytes - bytes)    ) - 1, 0) = previous_element.keep((word_bytes    ) - 1, bytes    );
    out_element.data(((word_bytes) * 8) - 1, (word_bytes - bytes) * 8) = current_element.data((bytes * 8) - 1, 0);
    out_element.keep(((word_bytes)    ) - 1, (word_bytes - bytes)    ) = current_element.keep((bytes    ) - 1, 0);

    out_element.last = current_element.last && current_element.keep(word_bytes - 1, bytes) == 0;

    if (previous_element_valid || out_element.last) {
        out.write(out_element);
    }

    if (current_element.last && !out_element.last) {
        // Flush remaining bytes
        current_element.data >>= bytes * 8;
        current_element.keep >>= bytes;
        out.write(current_element);
    }

    previous_element.data = current_element.data;
    previous_element.keep = current_element.keep;
    previous_element_valid = true;

    } while(!current_element.last);
}
