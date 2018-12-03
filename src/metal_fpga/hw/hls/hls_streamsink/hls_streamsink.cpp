#include <hls_common/mtl_stream.h>

void hls_streamsink(mtl_stream &data) {
    #pragma HLS INTERFACE axis port=data
    #pragma HLS INTERFACE ap_ctrl_none port=return

    while(true) {
        #pragma HLS pipeline
        data.read();
    }
}
