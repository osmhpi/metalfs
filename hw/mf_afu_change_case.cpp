#include "mf_afu_change_case.h"

#include <snap_types.h>

static uint64_t _mode = 0;

mf_retc_t afu_change_case_set_mode(uint64_t mode) {
    _mode = mode;
    return SNAP_RETC_SUCCESS;
}

void afu_change_case(mf_stream &in, mf_stream &out, snap_bool_t enable) {
    if (enable) {
        for (;;) {
            mf_stream_element element = in.read();

            if (_mode == 0) {
                for (int i = 0; i < sizeof(element.data); ++i)
                {
#pragma HLS unroll
                    snapu8_t tmp = element.data(i * 8 + 7, i * 8);
                    if (tmp >= 'a' && tmp <= 'z')
                        element.data(i * 8 + 7, i * 8) = tmp - ('a' - 'A');
                }
            } else {
                for (int i = 0; i < sizeof(element.data); ++i)
                {
#pragma HLS unroll
                    snapu8_t tmp = element.data(i * 8 + 7, i * 8);
                    if (tmp >= 'A' && tmp <= 'Z')
                        element.data(i * 8 + 7, i * 8) = tmp + ('a' - 'A');
                }
            }

            out.write(element);
            if (element.last) break;
        }
    }
}
