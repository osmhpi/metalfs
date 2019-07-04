#include <metal/stream.h>

void hls_streamgen(snapu32_t length, mtl_stream &out) {
    #pragma HLS INTERFACE s_axilite port=length bundle=ctrl offset=0x010
    #pragma HLS INTERFACE axis port=out
    #pragma HLS INTERFACE s_axilite port=return bundle=ctrl

    mtl_stream_element element;
    element.data = 0;
    element.keep = ~((mtl_stream_keep)0);
    element.last = false;

    snapu32_t bytes_written = 0;
    do {
    #pragma HLS pipeline
        bytes_written += sizeof(element.data);
        if (bytes_written >= length) {
            element.last = true;
            element.keep >>= (bytes_written - length);
        }

        out.write(element);
    } while (!element.last);
}
