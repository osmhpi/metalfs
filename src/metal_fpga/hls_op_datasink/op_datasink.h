#ifndef __MTL_OP_MEM_H__
#define __MTL_OP_MEM_H__

#include "mtl_definitions.h"
#include "mtl_stream.h"

// typedef struct mtl_mem_configuration {
//     uint64_t offset;
//     uint64_t size;
//     ap_uint<2> mode;
// } mtl_mem_configuration;

// mtl_retc_t op_mem_set_config(uint64_t offset, uint64_t size, uint64_t mode, mtl_mem_configuration &config);

// void op_mem_read(
//     snap_membus_t *din_gmem,
// #ifdef DRAM_ENABLED
//     snap_membus_t *din_ddrmem,
// #endif
//     mtl_stream &out,
//     mtl_mem_configuration &config);

// void op_mem_write(
//     mtl_stream &in,
//     snap_membus_t *dout_gmem,
// #ifdef DRAM_ENABLED
//     snap_membus_t *dout_ddrmem,
// #endif
//     mtl_mem_configuration &config);

// extern mtl_mem_configuration read_mem_config;
// extern mtl_mem_configuration write_mem_config;

#endif // __MTL_OP_MEM_H__
