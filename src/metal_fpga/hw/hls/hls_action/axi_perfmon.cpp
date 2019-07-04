#include "axi_perfmon.h"

#include <snap_types.h>
#include "mtl_endian.h"

namespace metal {
namespace fpga {

mtl_retc_t action_configure_perfmon(snapu32_t *perfmon_ctrl, uint8_t stream_id0, uint8_t stream_id1) {
    // Metrics Computed for AXI4-Stream Agent

    // 16
    // Transfer Cycle Count
    // Gives the total number of cycles the data is transferred.

    // 17
    // Packet Count
    // Gives the total number of packets transferred.

    // 18
    // Data Byte Count
    // Gives the total number of data bytes transferred.

    // 19
    // Position Byte Count
    // Gives the total number of position bytes transferred.

    // 20
    // Null Byte Count
    // Gives the total number of null bytes transferred.

    // 21
    // Slv_Idle_Cnt
    // Gives the number of idle cycles caused by the slave.

    // 22
    // Mst_Idle_Cnt
    // Gives the number of idle cycles caused by the master.

    // Write metric ids in reverse order to achieve the following slot mapping:
    // Slot 0 : 16
    // Slot 1 : 17
    // ...
    // Slot 6 : 22

    uint32_t metrics_0 = 0x15121110
        | stream_id0 << (0 * 8) + 5
        | stream_id0 << (1 * 8) + 5
        | stream_id0 << (2 * 8) + 5
        | stream_id0 << (3 * 8) + 5;

    uint32_t metrics_1 = 0x12111016
        | stream_id0 << (0 * 8) + 5
        | stream_id1 << (1 * 8) + 5
        | stream_id1 << (2 * 8) + 5
        | stream_id1 << (3 * 8) + 5;

    uint32_t metrics_2 = 0x00001615
        | stream_id1 << (0 * 8) + 5
        | stream_id1 << (1 * 8) + 5;

    // Write Metric Selection Register 0
    perfmon_ctrl[0x44 / sizeof(uint32_t)] = metrics_0;
    // Write Metric Selection Register 1
    perfmon_ctrl[0x48 / sizeof(uint32_t)] = metrics_1;
    // Write Metric Selection Register 2
    perfmon_ctrl[0x4C / sizeof(uint32_t)] = metrics_2;

    return SNAP_RETC_SUCCESS;
}

mtl_retc_t perfmon_reset(snapu32_t *perfmon_ctrl) {
    // Global_Clk_Cnt_Reset = 1
    // Metrics_Cnt_Reset = 1
    perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x00020002;

    // perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x00010001;

    return SNAP_RETC_SUCCESS;
}

mtl_retc_t perfmon_enable(snapu32_t *perfmon_ctrl) {
    // Enable Metric Counters
    // Global_Clk_Cnt_En = 1
    // Metrics_Cnt_En = 1
#ifndef NO_SYNTH
    perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x00010001;
#endif

    return SNAP_RETC_SUCCESS;
}

mtl_retc_t perfmon_disable(snapu32_t *perfmon_ctrl) {
    perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x0;

    return SNAP_RETC_SUCCESS;
}

mtl_retc_t action_perfmon_read(snap_membus_t *mem, snapu32_t *perfmon_ctrl) {
    snap_membus_t result = 0;

    // Global Clock Count
    mtl_set64<0 * 4>(result, (perfmon_ctrl[0x0000 / sizeof(uint32_t)], perfmon_ctrl[0x0004 / sizeof(uint32_t)]));

    mtl_set32<0 * 4 + 8>(result, perfmon_ctrl[(0x100 + 0 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<1 * 4 + 8>(result, perfmon_ctrl[(0x100 + 1 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<2 * 4 + 8>(result, perfmon_ctrl[(0x100 + 2 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<3 * 4 + 8>(result, perfmon_ctrl[(0x100 + 3 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<4 * 4 + 8>(result, perfmon_ctrl[(0x100 + 4 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<5 * 4 + 8>(result, perfmon_ctrl[(0x100 + 5 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<6 * 4 + 8>(result, perfmon_ctrl[(0x100 + 6 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<7 * 4 + 8>(result, perfmon_ctrl[(0x100 + 7 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<8 * 4 + 8>(result, perfmon_ctrl[(0x100 + 8 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);
    mtl_set32<9 * 4 + 8>(result, perfmon_ctrl[(0x100 + 9 * sizeof(uint32_t) * 4) / sizeof(uint32_t)]);

    *mem = result;

    return SNAP_RETC_SUCCESS;
}

}  // namespace fpga
}  // namespace metal
