#ifndef __MTL_AFU_FILE_H__
#define __MTL_AFU_FILE_H__

#include "mtl_definitions.h"
#include "mtl_stream.h"
#include "mtl_extmap.h"

// mtl_retc_t afu_file_set_read_buffer(uint64_t read_offset, uint64_t read_size);
// mtl_retc_t afu_file_set_write_buffer(uint64_t write_offset, uint64_t write_size);

// void afu_file_read(snap_membus_t *mem_ddr, mtl_stream &out, snap_bool_t enable);
// void afu_file_write(mtl_stream &in, snap_membus_t *mem_ddr, snap_bool_t enable);

extern mtl_extmap_t read_extmap;
extern mtl_extmap_t write_extmap;

#endif // __MTL_AFU_FILE_H__
