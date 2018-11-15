#ifndef __MTL_OP_MEM_H__
#define __MTL_OP_MEM_H__

#include "mtl_definitions.h"
#include "mtl_stream.h"

typedef struct axi_datamover_status {
    ap_uint<8> data;
    ap_uint<1> keep;
    ap_uint<1> last;
} axi_datamover_status_t;
typedef hls::stream<axi_datamover_status_t> axi_datamover_status_stream_t;

typedef ap_uint<103> axi_datamover_command_t;
typedef hls::stream<axi_datamover_command_t> axi_datamover_command_stream_t;

typedef struct mtl_mem_configuration {
    uint64_t offset;
    uint64_t size;
    ap_uint<2> mode;
} mtl_mem_configuration;

mtl_retc_t op_mem_set_config(uint64_t offset, uint64_t size, uint64_t mode, mtl_mem_configuration &config);

void op_mem_read(
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    mtl_mem_configuration &config);

void op_mem_write(
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_stream_t &s2mm_sts,
    mtl_mem_configuration &config);

extern mtl_mem_configuration read_mem_config;
extern mtl_mem_configuration write_mem_config;

#endif // __MTL_OP_MEM_H__
