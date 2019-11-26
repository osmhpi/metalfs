#include <snap_types.h>

#include <metal/stream.h>

void changecase(mtl_stream &in, mtl_stream &out, snapu32_t mode) {

    #pragma HLS INTERFACE axis port=in name=axis_input
    #pragma HLS INTERFACE axis port=out name=axis_output
    #pragma HLS INTERFACE s_axilite port=mode bundle=control offset=0x100
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    mtl_stream_element element;
    do {
        element = in.read();

        if (mode == 0) {
            for (int i = 0; i < sizeof(element.data); ++i)
            {
#pragma HLS unroll
                snapu8_t tmp = element.data(i * 8 + 7, i * 8);
                if (tmp >= 'a' && tmp <= 'z' && element.keep[i])
                    element.data(i * 8 + 7, i * 8) = tmp - ('a' - 'A');
            }
        } else {
            for (int i = 0; i < sizeof(element.data); ++i)
            {
#pragma HLS unroll
                snapu8_t tmp = element.data(i * 8 + 7, i * 8);
                if (tmp >= 'A' && tmp <= 'Z' && element.keep[i])
                    element.data(i * 8 + 7, i * 8) = tmp + ('a' - 'A');
            }
        }

        out.write(element);
    } while (!element.last);
}
