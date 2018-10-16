#include "mtl_op_passthrough.h"

void op_passthrough(mtl_stream &in, mtl_stream &out, snap_bool_t enable) {
	if (enable) {
		for (;;) {
			mtl_stream_element element = in.read();
			out.write(element);
			if (element.last) break;
		}
	}
}
