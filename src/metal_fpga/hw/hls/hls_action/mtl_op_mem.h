#pragma once

#include <snap_action_metal.h>
#include <metal/stream.h>
#include "mtl_definitions.h"

namespace metal {
namespace fpga {

typedef struct axi_datamover_status {
    ap_uint<8> data;
    ap_uint<1> keep;
    ap_uint<1> last;
} axi_datamover_status_t;
typedef hls::stream<axi_datamover_status_t> axi_datamover_status_stream_t;

typedef struct axi_datamover_ibtt_status {
    ap_uint<32> data;
    ap_uint<1> keep;
    ap_uint<1> last;
} axi_datamover_ibtt_status_t;
typedef hls::stream<axi_datamover_ibtt_status_t> axi_datamover_status_ibtt_stream_t;

typedef ap_uint<103> axi_datamover_command_t;
typedef hls::stream<axi_datamover_command_t> axi_datamover_command_stream_t;

mtl_retc_t op_mem_set_config(Address &address, snap_bool_t read, Address &config, snapu32_t *data_preselect_switch_ctrl);

void op_mem_read(
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    snapu32_t *random_ctrl,
    Address &config);

uint64_t op_mem_write(
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
    Address &config);

extern Address read_mem_config;
extern Address write_mem_config;

}  // namespace fpga
}  // namespace metal
