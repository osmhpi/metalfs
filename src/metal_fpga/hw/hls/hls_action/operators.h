#pragma once

#include <hls_stream.h>
#include <hls_snap.H>
#include "mtl_definitions.h"
#include "mtl_op_mem.h"

namespace metal {
namespace fpga {

const snapu32_t PerfmonBaseAddr = 0x44A10000 / sizeof(uint32_t);
const snapu32_t DataPreselectSwitchBaseAddr = 0x44A20000 / sizeof(uint32_t);
const snapu32_t RandomBaseAddr = 0x44A30000 / sizeof(uint32_t);
const snapu32_t ImageInfoBaseAddr = 0x44A00000 / sizeof(uint32_t);
const snapu32_t SwitchBaseAddr = 0x44A40000 / sizeof(uint32_t);
const snapu32_t OperatorBaseAddr = 0x44A50000 / sizeof(uint32_t);
const snapu32_t OperatorOffset = 0x10000 / sizeof(uint32_t);

void clear_operator_interrupts(hls::stream<snapu8_t> &interrupt_reg, snapu32_t *metal_ctrl);

void action_run_operators(
    snap_membus_t * mem_in,
    snap_membus_t * mem_out,
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
    snapu32_t *metal_ctrl,
    hls::stream<snapu8_t> &interrupt_reg,
    snapu64_t enable_mask,
    snapu64_t *bytes_written
);

}  // namespace fpga
}  // namespace metal
