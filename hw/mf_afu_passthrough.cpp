#include "mf_afu_passthrough.h"

void afu_passthrough(mf_stream &in, mf_stream &out, snap_bool_t enable) {
	if (enable) {
		for (;;) {
			mf_stream_element element = in.read();
			out.write(element);
			if (element.last) break;
		}
	}
}
