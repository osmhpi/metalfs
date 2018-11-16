#ifndef __MTL_OP_CHANGE_CASE_H__
#define __MTL_OP_CHANGE_CASE_H__

#include "mtl_definitions.h"
#include <hls_common/mtl_stream.h>

mtl_retc_t op_change_case_set_mode(uint64_t mode);

void op_change_case(mtl_stream &in, mtl_stream &out, snap_bool_t enable);

#endif // __MTL_OP_CHANGE_CASE_H__
