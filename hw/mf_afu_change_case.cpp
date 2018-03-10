#include "mf_afu_change_case.h"

#include <snap_types.h>

static uint64_t _mode = 0;

mf_retc_t afu_change_case_set_mode(uint64_t mode) {
    _mode = mode;
    return SNAP_RETC_SUCCESS;
}

void afu_change_case_impl(mf_byte_stream &in, mf_byte_stream &out, snap_bool_t enable) {
    if (enable) {
        for (;;) {
            byte_stream_element element = in.read();

            if (_mode == 0) {
                if (element.data >= 'a' && element.data <= 'z')
                    element.data = element.data - ('a' - 'A');
            } else {
                if (element.data >= 'A' && element.data <= 'Z')
                    element.data = element.data + ('a' - 'A');
            }

            out.write(element);
            if (element.last) break;
        }
    }
}

void afu_change_case(mf_stream &in, mf_stream &out, snap_bool_t enable) {
#pragma HLS dataflow
    mf_byte_stream bytes_in, bytes_out;
    stream_words2bytes<mf_stream_element, false>(bytes_in, in, enable);
    afu_change_case_impl(bytes_in, bytes_out, enable);
    stream_bytes2words<mf_stream_element, false>(out, bytes_out, enable);
}
