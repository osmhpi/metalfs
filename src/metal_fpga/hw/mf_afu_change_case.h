#ifndef __MF_AFU_CHANGE_CASE_H__
#define __MF_AFU_CHANGE_CASE_H__

#include "mf_definitions.h"
#include "mf_stream.h"

mf_retc_t afu_change_case_set_mode(uint64_t mode);

void afu_change_case(mf_stream &in, mf_stream &out, snap_bool_t enable);

#endif // __MF_AFU_CHANGE_CASE_H__
