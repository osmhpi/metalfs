#pragma once

#include <hls_stream.h>
#include <hls_snap.H>
#include "action_metalfpga.H"
#include "mtl_definitions.h"
#include "mtl_op_mem.h"

namespace metal {
namespace fpga {

mtl_retc_t process_action(snap_membus_t * mem_in,
                          snap_membus_t * mem_out,
#ifdef NVME_ENABLED
                          NVMeCommandStream &nvme_read_cmd,
                          NVMeResponseStream &nvme_read_resp,
                          NVMeCommandStream &nvme_write_cmd,
                          NVMeResponseStream &nvme_write_resp,
#endif
                          axi_datamover_command_stream_t &mm2s_cmd,
                          axi_datamover_status_stream_t &mm2s_sts,
                          axi_datamover_command_stream_t &s2mm_cmd,
                          axi_datamover_status_ibtt_stream_t &s2mm_sts,
                          snapu32_t *metal_ctrl,
                          hls::stream<snapu8_t> &interrupt_reg,
                          action_reg * act_reg);

}  // namespace fpga
}  // namespace metal
