#include <metal/stream.h>

void hls_streamsink(mtl_stream &data) {
    #pragma HLS INTERFACE axis port=data
    #pragma HLS INTERFACE ap_ctrl_none port=return

    while (true) {
        #pragma HLS pipeline
        data.read();
    }
}

#ifndef __SYNTHESIS__
// No tests specified
int main(int argc, char* argv[]) { return 0; }
#endif
