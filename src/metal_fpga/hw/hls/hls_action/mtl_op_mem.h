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

class DatamoverIbttStatus : public ap_uint<32> {
public:
    auto end_of_packet()         -> decltype(ap_uint<32>::operator[](31)) { return (*this)[31]; }
    auto num_bytes_transferred() -> decltype(ap_uint<32>::range(30, 8))   { return this->range(30, 8); }
};

typedef struct axi_datamover_ibtt_status {
    DatamoverIbttStatus data;
    ap_uint<1> keep;
    ap_uint<1> last;
} axi_datamover_ibtt_status_t;
typedef hls::stream<axi_datamover_ibtt_status_t> axi_datamover_status_ibtt_stream_t;

class DatamoverCommand : public ap_uint<103> {
public:
    auto end_of_frame()      -> decltype(ap_uint<103>::operator[](30)) { return (*this)[30]; }
    auto axi_burst_type()    -> decltype(ap_uint<103>::operator[](23)) { return (*this)[23]; }
    auto effective_address() -> decltype(ap_uint<103>::range(95, 32))  { return this->range(95, 32); }
    auto transfer_bytes()    -> decltype(ap_uint<103>::range(22, 0))   { return this->range(22, 0); }
};

typedef hls::stream<DatamoverCommand> axi_datamover_command_stream_t;

class NVMeCommand : public ap_uint<168> {
public:
    auto dram_offset()          -> decltype(ap_uint<168>::range(63, 0))     { return this->range(63, 0); }
    auto nvme_block_offset()    -> decltype(ap_uint<168>::range(127, 64))   { return this->range(127, 64); }
    auto num_blocks()           -> decltype(ap_uint<168>::range(159, 128))  { return this->range(159, 128); }
    auto drive()                -> decltype(ap_uint<168>::range(162, 160))  { return this->range(162, 160); }
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

#ifdef NVME_ENABLED
void preload_nvme_blocks(const Address &address, mtl_extmap_t &map, NVMeCommandStream &nvme_read_cmd, NVMeResponseStream &nvme_read_resp);
#endif

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
