#ifndef __MTL_OP_PASSTHROUGH_H__
#define __MTL_OP_PASSTHROUGH_H__

#include <hls_common/mtl_stream.h>

void op_passthrough(mtl_stream &in, mtl_stream &out, snap_bool_t enable);

#endif // __MTL_OP_PASSTHROUGH_H__
