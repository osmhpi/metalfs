#ifndef __MF_AFU_FILE_H__
#define __MF_AFU_FILE_H__

#include "mf_definitions.h"
#include "mf_stream.h"

mf_retc_t afu_file_set_read_buffer(uint64_t read_offset, uint64_t read_size);
mf_retc_t afu_file_set_write_buffer(uint64_t write_offset, uint64_t write_size);

void afu_file_read(snap_membus_t *mem_ddr, mf_stream &out, snap_bool_t enable);
void afu_file_write(mf_stream &in, snap_membus_t *mem_ddr, snap_bool_t enable);

#endif // __MF_AFU_FILE_H__
