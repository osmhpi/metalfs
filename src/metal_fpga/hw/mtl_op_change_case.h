#ifndef __MTL_AFU_CHANGE_CASE_H__
#define __MTL_AFU_CHANGE_CASE_H__

#include "mtl_definitions.h"
#include "mtl_stream.h"

mtl_retc_t afu_change_case_set_mode(uint64_t mode);

void afu_change_case(mtl_stream &in, mtl_stream &out, snap_bool_t enable);

#endif // __MTL_AFU_CHANGE_CASE_H__
