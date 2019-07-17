#pragma once

#include <snap_action_metal.h>
#include <metal/stream.h>
#include "mtl_definitions.h"
#include "mtl_extmap.h"

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

struct NVMeCommand {
    ap_uint<64> dram_offset;
    ap_uint<64> nvme_block_offset;
    ap_uint<64> num_blocks;
};

typedef hls::stream<NVMeCommand> NVMeCommandStream;
typedef hls::stream<ap_uint<1>> NVMeResponseStream;

struct MemoryTransferResult {
    snap_bool_t last;
    snapu32_t num_bytes;
};

extern mtl_extmap_t nvme_read_extmap;
extern mtl_extmap_t nvme_write_extmap;

mtl_retc_t op_mem_set_config(Address &address, snap_bool_t read, Address &config, snapu32_t *data_preselect_switch_ctrl);

void op_mem_read(
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    snapu32_t *random_ctrl,
#ifdef NVME_ENABLED
    NVMeCommandStream &nvme_read_cmd,
    NVMeResponseStream &nvme_read_resp,
#endif
    Address &config);

uint64_t op_mem_write(
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
#ifdef NVME_ENABLED
    NVMeCommandStream &nvme_write_cmd,
    NVMeResponseStream &nvme_write_resp,
#endif
    Address &config);

extern Address read_mem_config;
extern Address write_mem_config;

}  // namespace fpga
}  // namespace metal
