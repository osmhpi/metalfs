#ifndef __MTL_STREAM_H__
#define __MTL_STREAM_H__

#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <hls_snap.H>

struct byte_stream_element {
    snapu8_t data;
    snap_bool_t last;
};
typedef hls::stream<byte_stream_element> mtl_byte_stream;

template<uint8_t NB>
struct stream_element {
    ap_uint<8 * NB> data;
    ap_uint<NB> keep;
    snap_bool_t last;
};

typedef stream_element<8> mtl_stream_element;
typedef hls::stream<mtl_stream_element> mtl_stream;

#endif // __MTL_STREAM_H__
