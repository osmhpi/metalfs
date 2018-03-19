#ifndef __MF_AFU_MEM_H__
#define __MF_AFU_MEM_H__

#include "mf_definitions.h"
#include "mf_stream.h"

mf_retc_t afu_mem_set_read_buffer(uint64_t read_offset, uint64_t read_size);
mf_retc_t afu_mem_set_write_buffer(uint64_t write_offset, uint64_t write_size);

void afu_mem_read(snap_membus_t *din_gmem, mf_stream &out, snap_bool_t enable);
void afu_mem_write(mf_stream &in, snap_membus_t *dout_gmem, snap_bool_t enable);

#endif // __MF_AFU_MEM_H__
