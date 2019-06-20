#include "operators.h"

#include <snap_types.h>

#include "mtl_endian.h"

const snapu32_t OperatorBaseAddr = 0x44A40000 / sizeof(uint32_t);
const snapu32_t OperatorOffset = 0x10000 / sizeof(uint32_t);

void clear_operator_interrupts(hls::stream<snapu8_t> &interrupt_reg, snapu32_t *metal_ctrl) {
    snapu32_t *operator_ctrl =
        (metal_ctrl + OperatorBaseAddr);

    snapu8_t set_interrupts = interrupt_reg.read();
    for (int i = 0; i < 8; ++i) {
        if (set_interrupts & 1 << i) {
            // 0x0c : IP Interrupt Status Register (Read/TOW)
            //        bit 0  - Channel 0 (ap_done)
            //        others - reserved
            operator_ctrl[i * OperatorOffset + 3] = 1;

            // Wait until operator has taken back the interrupt
            while (interrupt_reg.read() & 1 << i);
        }
    }
}

static void poll_interrupts(snapu8_t mask, hls::stream<snapu8_t> &interrupt_reg) {
    if (mask != 1 && mask != 0) {
        while (interrupt_reg.read() != (ap_uint<1>(0), mask(7, 1)));
    }
}

void do_run_operators(
    snap_membus_t * mem_in,
    snap_membus_t * mem_out,
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
    snapu32_t *random_ctrl,
    snapu64_t &enable_mask,
    snapu64_t *bytes_written
) {
    {
#pragma HLS DATAFLOW
        // The order should only matter when executing in the test bench

        // Input Operators
        op_mem_read(
            mm2s_cmd,
            mm2s_sts,
            random_ctrl,
            read_mem_config);

        // Output Operators
        *bytes_written = op_mem_write(
            s2mm_cmd,
            s2mm_sts,
            write_mem_config);
    }
}

void do_configure_and_run_operators(
    snap_membus_t * mem_in,
    snap_membus_t * mem_out,
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
    snapu32_t *metal_ctrl,
    snapu64_t &enable_mask,
    snapu64_t *bytes_written
) {
    snapu32_t *random_ctrl =
        (metal_ctrl + (0x44A20000 / sizeof(uint32_t)));

    snapu32_t *operator_ctrl =
        (metal_ctrl + OperatorBaseAddr);

    // From the Vivado HLS User Guide:
    // 0x00 : Control signals
    //        bit 0  - ap_start (Read/Write/SC)
    //        bit 1  - ap_done (Read/COR)
    //        bit 2  - ap_idle (Read)
    //        bit 3  - ap_ready (Read)
    //        bit 7  - auto_restart (Read/Write)
    //        others - reserved
    // 0x04 : Global Interrupt Enable Register
    //        bit 0  - Global Interrupt Enable (Read/Write)
    //        others - reserved
    // 0x08 : IP Interrupt Enable Register (Read/Write)
    //        bit 0  - Channel 0 (ap_done)
    //        bit 1  - Channel 1 (ap_ready)
    // 0x0c : IP Interrupt Status Register (Read/TOW)
    //        bit 0  - Channel 0 (ap_done)
    //        others - reserved

    for (int i = 0; i < 8; ++i) {
        if (enable_mask & 1 << (i + 1)) {
            operator_ctrl[i * OperatorOffset + 0] = 1;
            operator_ctrl[i * OperatorOffset + 1] = 1;
            operator_ctrl[i * OperatorOffset + 2] = 1;
        }
    }

    if (enable_mask[0]) {
        // This triggers the data source and sink (not enabled in preparation mode)
        do_run_operators(mem_in, mem_out, mm2s_cmd, mm2s_sts, s2mm_cmd, s2mm_sts, random_ctrl, enable_mask, bytes_written);
    }

    for (int i = 0; i < 8; ++i) {
        if (enable_mask & 1 << (i + 1)) {
            // Disable prepare (if applicable to operator)
            operator_ctrl[i * OperatorOffset + 0x10 / sizeof(snapu32_t)] = 0;
        }
    }
}

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
) {
    #pragma HLS DATAFLOW
    poll_interrupts(enable_mask, interrupt_reg);
    do_configure_and_run_operators(mem_in, mem_out, mm2s_cmd, mm2s_sts, s2mm_cmd, s2mm_sts, metal_ctrl, enable_mask, bytes_written);
}
