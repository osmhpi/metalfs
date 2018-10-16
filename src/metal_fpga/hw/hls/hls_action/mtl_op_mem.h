#ifndef __MTL_OP_MEM_H__
#define __MTL_OP_MEM_H__

#include "mtl_definitions.h"
#include "mtl_stream.h"

typedef struct mtl_mem_configuration {
    uint64_t offset;
    uint64_t size;
} mtl_mem_configuration;

mtl_retc_t op_mem_set_config(uint64_t offset, uint64_t size, mtl_mem_configuration &config);

void op_mem_read(snap_membus_t *din_gmem, mtl_stream &out, mtl_mem_configuration &config, snap_bool_t enable);
void op_mem_write(mtl_stream &in, snap_membus_t *dout_gmem, mtl_mem_configuration &config, snap_bool_t enable);
void op_mem_readwrite(
    mtl_stream &in,
    mtl_stream &out,
    snap_membus_t *mem,
    mtl_mem_configuration &read_config,
    mtl_mem_configuration &write_config,
    snap_bool_t read_enable,
    snap_bool_t write_enable);

extern mtl_mem_configuration read_mem_config;
extern mtl_mem_configuration write_mem_config;
extern mtl_mem_configuration read_ddr_mem_config;
extern mtl_mem_configuration write_ddr_mem_config;

#endif // __MTL_OP_MEM_H__
