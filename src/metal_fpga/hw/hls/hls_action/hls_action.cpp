#include "action_metalfpga.H"

#include "operators.h"
#include "axi_perfmon.h"
#include "axi_switch.h"
#include "mtl_jobs.h"
#include "mtl_endian.h"
#include "mtl_file.h"
#include "mtl_jobstruct.h"
#include <hls_common/mtl_stream.h>

#include "mtl_op_mem.h"
#include "mtl_op_file.h"

#define HW_RELEASE_LEVEL       0x00000013

// ------------------------------------------------
// -------------- ACTION ENTRY POINT --------------
// ------------------------------------------------
void hls_action(snap_membus_t * din,
                snap_membus_t * dout,
#ifdef NVME_ENABLED
                snapu32_t * nvme,
#endif
                axi_datamover_command_stream_t &mm2s_cmd,
                axi_datamover_status_stream_t &mm2s_sts,
                axi_datamover_command_stream_t &s2mm_cmd,
                axi_datamover_status_ibtt_stream_t &s2mm_sts,
                snapu32_t *metal_ctrl,
                hls::stream<snapu8_t> &interrupt_reg,
                action_reg * action_reg,
                action_RO_config_reg * action_config)
{
    // Configure Host Memory AXI Interface
#pragma HLS INTERFACE m_axi port=din bundle=host_mem offset=slave depth=512 \
    max_read_burst_length=64 max_write_burst_length=64
#pragma HLS INTERFACE s_axilite port=din bundle=ctrl_reg offset=0x030

#pragma HLS INTERFACE m_axi port=dout bundle=host_mem offset=slave depth=512 \
    max_read_burst_length=64 max_write_burst_length=64
#pragma HLS INTERFACE s_axilite port=dout bundle=ctrl_reg offset=0x040

    // Configure Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=action_config
#pragma HLS INTERFACE s_axilite port=action_config bundle=ctrl_reg  offset=0x010
#pragma HLS DATA_PACK variable=action_reg
#pragma HLS INTERFACE s_axilite port=action_reg bundle=ctrl_reg offset=0x100
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_reg

#pragma HLS INTERFACE axis port=interrupt_reg

#ifdef NVME_ENABLED
    //NVME Config Interface
#pragma HLS INTERFACE m_axi port=nvme bundle=nvme //offset=slave
//#pragma HLS INTERFACE s_axilite port=nvme bundle=ctrl_reg offset=0x060
#endif

    // Configure AXI4 Stream Interface
#pragma HLS INTERFACE axis port=mm2s_cmd
#pragma HLS INTERFACE axis port=mm2s_sts
#pragma HLS INTERFACE axis port=s2mm_cmd
#pragma HLS INTERFACE axis port=s2mm_sts

#pragma HLS INTERFACE m_axi port=metal_ctrl depth=32

    // Required Action Type Detection
    switch (action_reg->Control.flags) {
    case 0:
        action_config->action_type = (snapu32_t)METALFPGA_ACTION_TYPE;
        action_config->release_level = (snapu32_t)HW_RELEASE_LEVEL;
        action_reg->Control.Retc = (snapu32_t)0xe00f;
        break;
    default:
        action_reg->Control.Retc = process_action(
            din,
            dout,
#ifdef NVME_ENABLED
            nvme,
#endif
            mm2s_cmd,
            mm2s_sts,
            s2mm_cmd,
            s2mm_sts,
            metal_ctrl,
            interrupt_reg,
            action_reg);
        break;
    }
}

#include "testbench.c"
