#ifndef __MF_AFU_MEM_H__
#define __MF_AFU_MEM_H__

#include "mf_definitions.h"
#include "mf_stream.h"

typedef struct mf_mem_configuration {
    uint64_t offset;
    uint64_t size;
} mf_mem_configuration;

mf_retc_t afu_mem_set_config(uint64_t offset, uint64_t size, mf_mem_configuration &config);

void afu_mem_read(snap_membus_t *din_gmem, mf_stream &out, mf_mem_configuration &config, snap_bool_t enable);
void afu_mem_write(mf_stream &in, snap_membus_t *dout_gmem, mf_mem_configuration &config, snap_bool_t enable);

extern mf_mem_configuration read_mem_config;
extern mf_mem_configuration write_mem_config;
extern mf_mem_configuration read_ddr_mem_config;
extern mf_mem_configuration write_ddr_mem_config;

#endif // __MF_AFU_MEM_H__
