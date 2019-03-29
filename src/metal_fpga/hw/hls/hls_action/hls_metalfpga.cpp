#include "action_metalfpga.H"

#include "axi_switch.h"
#include "mtl_endian.h"
#include "mtl_file.h"
#include "mtl_jobstruct.h"
#include <hls_common/mtl_stream.h>

#include "mtl_op_mem.h"
#include "mtl_op_file.h"

#define HW_RELEASE_LEVEL       0x00000013

static mtl_retc_t process_action(snap_membus_t * mem_in,
                                snap_membus_t * mem_out,
#ifdef NVME_ENABLED
                                snapu32_t * nvme,
#endif
                                axi_datamover_command_stream_t &mm2s_cmd,
                                axi_datamover_status_stream_t &mm2s_sts,
                                axi_datamover_command_stream_t &s2mm_cmd,
                                axi_datamover_status_stream_t &s2mm_sts,
                                snapu32_t *metal_ctrl,
                                action_reg * act_reg);

static mtl_retc_t action_map(snap_membus_t * mem_in, const mtl_job_map_t & job);
static mtl_retc_t action_mount(snapu32_t * nvme, const mtl_job_fileop_t & job);
static mtl_retc_t action_writeback(snapu32_t * nvme, const mtl_job_fileop_t & job);

static mtl_retc_t action_configure_streams(snapu32_t *switch_ctrl, snap_membus_t * mem_out, const uint64_t job_address);
static mtl_retc_t action_configure_perfmon(snapu32_t *perfmon_ctrl, uint8_t stream_id0, uint8_t stream_id1);
static mtl_retc_t action_perfmon_read(snap_membus_t *mem, snapu32_t *perfmon_ctrl);
static mtl_retc_t action_run_operators(
    snap_membus_t * mem_in,
    snap_membus_t * mem_out,
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_stream_t &s2mm_sts,
    snapu32_t *random_ctrl
);

// ------------------------------------------------
// -------------- ACTION ENTRY POINT --------------
// ------------------------------------------------
// This design uses FPGA DDR.
// Set Environment Variable "SDRAM_USED=TRUE" before compilation.
void hls_action(snap_membus_t * din,
                snap_membus_t * dout,
#ifdef NVME_ENABLED
                snapu32_t * nvme,
#endif
                axi_datamover_command_stream_t &mm2s_cmd,
                axi_datamover_status_stream_t &mm2s_sts,
                axi_datamover_command_stream_t &s2mm_cmd,
                axi_datamover_status_stream_t &s2mm_sts,
                snapu32_t *metal_ctrl,
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
            action_reg);
        break;
    }
}


// ------------------------------------------------
// --------------- ACTION FUNCTIONS ---------------
// ------------------------------------------------

static mtl_retc_t perfmon_reset(snapu32_t *perfmon_ctrl);
static mtl_retc_t perfmon_enable(snapu32_t *perfmon_ctrl);
static mtl_retc_t perfmon_disable(snapu32_t *perfmon_ctrl);

// Decode job_type and call appropriate action
static mtl_retc_t process_action(snap_membus_t * mem_in,
                                snap_membus_t * mem_out,
#ifdef NVME_ENABLED
                                snapu32_t * nvme,
#endif
                                axi_datamover_command_stream_t &mm2s_cmd,
                                axi_datamover_status_stream_t &mm2s_sts,
                                axi_datamover_command_stream_t &s2mm_cmd,
                                axi_datamover_status_stream_t &s2mm_sts,
                                snapu32_t *metal_ctrl,
                                action_reg * act_reg)
{

    snapu32_t *perfmon_ctrl =
        (metal_ctrl + (0x44A00000 / sizeof(uint32_t)));
    snapu32_t *data_preselect_switch_ctrl =
        (metal_ctrl + (0x44A10000 / sizeof(uint32_t)));
    snapu32_t *random_ctrl =
        (metal_ctrl + (0x44A20000 / sizeof(uint32_t)));
    snapu32_t *switch_ctrl =
        (metal_ctrl + (0x44A30000 / sizeof(uint32_t)));

    mtl_retc_t result = SNAP_RETC_SUCCESS;

    switch (act_reg->Data.job_type) {
    case MTL_JOB_MAP:
    {
#ifdef NVME_ENABLED
        mtl_job_map_t map_job = mtl_read_job_map(mem_in, act_reg->Data.job_address);
        result = action_map(mem_in, map_job);
#else
        result = SNAP_RETC_SUCCESS;
#endif
        break;
    }
    case MTL_JOB_MOUNT:
    {
#ifdef NVME_ENABLED
        mtl_job_fileop_t mount_job = mtl_read_job_fileop(mem_in, act_reg->Data.job_address);
        result = action_mount(nvme, mount_job);
#else
        result = SNAP_RETC_SUCCESS;
#endif
        break;
    }
    case MTL_JOB_WRITEBACK:
    {
#ifdef NVME_ENABLED
        mtl_job_fileop_t writeback_job = mtl_read_job_fileop(mem_in, act_reg->Data.job_address);
        result = action_writeback(nvme, writeback_job);
#else
        result = SNAP_RETC_SUCCESS;
#endif
        break;
    }
    case MTL_JOB_CONFIGURE_STREAMS:
    {
        result = action_configure_streams(switch_ctrl, mem_in, act_reg->Data.job_address);
        break;
    }
    case MTL_JOB_RESET_PERFMON:
    {
        perfmon_reset(perfmon_ctrl);
        result = SNAP_RETC_SUCCESS;
        break;
    }
    case MTL_JOB_CONFIGURE_PERFMON:
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        result = action_configure_perfmon(perfmon_ctrl, mtl_get64<0>(line), mtl_get64<8>(line));
        break;
    }
    case MTL_JOB_READ_PERFMON_COUNTERS:
    {
        result = action_perfmon_read(mem_in + MFB_ADDRESS(act_reg->Data.job_address), perfmon_ctrl);
        break;
    }
    case MTL_JOB_RUN_OPERATORS:
    {
        perfmon_enable(perfmon_ctrl);

        action_run_operators(
            mem_in,
            mem_out,
            mm2s_cmd,
            mm2s_sts,
            s2mm_cmd,
            s2mm_sts,
            random_ctrl
        );
        // perfmon_disable(perfmon_ctrl);
    #ifndef NO_SYNTH
        perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x0;
    #endif

        result = SNAP_RETC_SUCCESS;
        break;
    }
    case MTL_JOB_OP_MEM_SET_READ_BUFFER:
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        result = op_mem_set_config(mtl_get64<0>(line), mtl_get64<8>(line), mtl_get64<16>(line), true, read_mem_config, data_preselect_switch_ctrl);
        break;
    }
    case MTL_JOB_OP_MEM_SET_WRITE_BUFFER:
    {
        snap_membus_t line = mem_in[MFB_ADDRESS(act_reg->Data.job_address)];
        result = op_mem_set_config(mtl_get64<0>(line), mtl_get64<8>(line), mtl_get64<16>(line), false, write_mem_config, data_preselect_switch_ctrl);
        break;
    }
    default:
        result = SNAP_RETC_FAILURE;
        break;
    }

    return result;
}


static snapu64_t _enable_mask = 0;

static mtl_retc_t action_configure_streams(snapu32_t *switch_ctrl, snap_membus_t * mem_in, const uint64_t job_address) {
    // Everything fits into one memory line
    snap_membus_t line = mem_in[MFB_ADDRESS(job_address)];

    _enable_mask = mtl_get64(line, 0);

    switch_set_mapping(switch_ctrl, mtl_get32(line,  8), 0);
    switch_set_mapping(switch_ctrl, mtl_get32(line, 12), 1);
    switch_set_mapping(switch_ctrl, mtl_get32(line, 16), 2);
    switch_set_mapping(switch_ctrl, mtl_get32(line, 20), 3);
    switch_set_mapping(switch_ctrl, mtl_get32(line, 24), 4);
    switch_set_mapping(switch_ctrl, mtl_get32(line, 28), 5);
    switch_set_mapping(switch_ctrl, mtl_get32(line, 32), 6);
    switch_set_mapping(switch_ctrl, mtl_get32(line, 36), 7);
    switch_commit(switch_ctrl);

    return SNAP_RETC_SUCCESS;
}

static mtl_retc_t action_configure_perfmon(snapu32_t *perfmon_ctrl, uint8_t stream_id0, uint8_t stream_id1) {
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

static mtl_retc_t perfmon_reset(snapu32_t *perfmon_ctrl) {
    // Global_Clk_Cnt_Reset = 1
    // Metrics_Cnt_Reset = 1
    perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x00020002;

    // perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x00010001;

    return SNAP_RETC_SUCCESS;
}

static mtl_retc_t perfmon_enable(snapu32_t *perfmon_ctrl) {
    // Enable Metric Counters
    // Global_Clk_Cnt_En = 1
    // Metrics_Cnt_En = 1
#ifndef NO_SYNTH
    perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x00010001;
#endif

    return SNAP_RETC_SUCCESS;
}

static mtl_retc_t perfmon_disable(snapu32_t *perfmon_ctrl) {
    perfmon_ctrl[0x300 / sizeof(uint32_t)] = 0x0;

    return SNAP_RETC_SUCCESS;
}

static mtl_retc_t action_perfmon_read(snap_membus_t *mem, snapu32_t *perfmon_ctrl) {
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

static mtl_retc_t action_run_operators(
    snap_membus_t * mem_in,
    snap_membus_t * mem_out,
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_stream_t &s2mm_sts,
    snapu32_t *random_ctrl
) {
    snap_bool_t enable_0 = _enable_mask[0];
    snap_bool_t enable_1 = _enable_mask[1];
    snap_bool_t enable_2 = _enable_mask[2];
    snap_bool_t enable_3 = _enable_mask[3];
    snap_bool_t enable_4 = _enable_mask[4];
    snap_bool_t enable_5 = _enable_mask[5];
    snap_bool_t enable_6 = _enable_mask[6];
    snap_bool_t enable_7 = _enable_mask[7];
    snap_bool_t enable_8 = _enable_mask[8];
    snap_bool_t enable_9 = _enable_mask[9];

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
        op_mem_write(
            s2mm_cmd,
            s2mm_sts,
            write_mem_config);
    }

    return SNAP_RETC_SUCCESS;
}


//-----------------------------------------------------------------------------
//--- TESTBENCH ---------------------------------------------------------------
//-----------------------------------------------------------------------------

//#ifdef NO_SYNTH
#ifdef FALSE

#include <stdio.h>
#include <endian.h>

int test_op_mem_read() {
    static snap_membus_t src_mem[16];
    static uint8_t *src_memb = (uint8_t *) src_mem;
    static mtl_stream out_stream;

    // Fill the memory with data
    for (int i = 0; i < 1024; ++i) {
        src_memb[i] = i % 256;
    }

    bool success = true;
    uint64_t offset, size;

    {
        // Op mem read should stream entire memory line
        offset = 0, size = 64;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mtl_stream_element result;
        for (int element = 0; element < 8; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream two entire memory lines
        offset = 0, size = 128;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mtl_stream_element result;
        for (int element = 0; element < 2 * 8; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream a half memory line starting at offset
        offset = 16, size = 32;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mtl_stream_element result;
        for (int element = 0; element < 8 / 2; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream one and a half memory line starting at offset
        offset = 16, size = 96;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        mtl_stream_element result;
        for (int element = 0; element < 12; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream one and a half memory line
        offset = 0, size = 96;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        // A memory line produces 8 stream elements
        mtl_stream_element result;
        for (int element = 0; element < 12; ++element) {
            result = out_stream.read();
            success &= memcmp(&result.data, &src_memb[element * 8 + offset], 8) == 0;
            success &= result.strb == 0xff;
        }
        success &= result.last == true;

        if (!success) return -1;
    }

    {
        // Op mem read should stream a half stream word starting at offset
        offset = 16, size = 4;
        op_mem_set_config(offset, size, read_mem_config);
        op_mem_read(src_mem, out_stream, read_mem_config, true);

        mtl_stream_element result = out_stream.read();
        success &= memcmp(&result.data, &src_memb[offset], 4) == 0;
        success &= result.strb == 0x0f;
        success &= result.last == true;

        if (!success) return -1;
    }

    return 0;
}

int test_op_mem_write() {
    static mtl_stream in_stream;
    static snap_membus_t dest_mem[16];
    static uint8_t *dest_memb = (uint8_t *) dest_mem;
    static mtl_stream_element stream_elements[128];
    static uint8_t reference_data[1024];

    // Fill the stream elements with data
    for (int i = 0; i < 1024; ++i) {
        reference_data[i] = i % 256;
    }
    for (int i = 0; i < 128; ++i) {
        uint8_t *data = (uint8_t*) &stream_elements[i].data;
        for (int j = 0; j < 8; ++j) {
            data[j] = (i * 8 + j) % 256;
        }
    }

    bool success = true;
    uint64_t offset, size;

    {
        // Op mem write should store entire memory line
        offset = 0, size = 64;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 8; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 7;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // Op mem write should store two entire memory lines
        offset = 0, size = 128;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 16; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 15;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // Op mem write should store a half memory line
        offset = 0, size = 32;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 4; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 3;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // Op mem write should store a half memory line at offset
        offset = 16, size = 32;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 4; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 3;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    {
        // Op mem write should store one and a half memory lines at offset
        offset = 16, size = 96;
        memset(dest_memb, 0, 1024);

        // 8 stream elements make up a memory line
        for (int element = 0; element < 12; ++element) {
            mtl_stream_element e = stream_elements[element];
            e.strb = 0xff;
            e.last = element == 11;
            in_stream.write(e);
        }

        op_mem_set_config(offset, size, write_mem_config);
        op_mem_write(in_stream, dest_mem, write_mem_config, true);

        success &= memcmp(reference_data, &dest_memb[offset], size) == 0;

        if (!success) return -1;
    }

    return 0;
}

int main()
{
    // Poor man's unit tests:
    if (test_op_mem_read()) printf("Test failure\n"); else printf("Test success\n");
    if (test_op_mem_write()) printf("Test failure\n"); else printf("Test success\n");

    static snap_membus_t host_gmem[1024];
    static snap_membus_t dram_gmem[1024];
    //static snapu32_t nvme_gmem[];
    action_reg act_reg;
    action_RO_config_reg act_config;

    // Streams
    static snapu32_t switch_ctrl[32];
    static mtl_stream stream_0_2;
    static mtl_stream stream_1_1; // Shortcut for disabled op
    static mtl_stream stream_2_3;
    static mtl_stream stream_3_4;
    static mtl_stream stream_4_5;
    static mtl_stream stream_5_6;
    static mtl_stream stream_6_7;
    static mtl_stream stream_7_0;

    mtl_stream &axis_s_0 = stream_7_0;
    mtl_stream &axis_s_1 = stream_1_1;
    mtl_stream &axis_s_2 = stream_0_2;
    mtl_stream &axis_s_3 = stream_2_3;
    mtl_stream &axis_s_4 = stream_3_4;
    mtl_stream &axis_s_5 = stream_4_5;
    mtl_stream &axis_s_6 = stream_5_6;
    mtl_stream &axis_s_7 = stream_6_7;

    mtl_stream &axis_m_0 = stream_0_2;
    mtl_stream &axis_m_1 = stream_1_1;
    mtl_stream &axis_m_2 = stream_2_3;
    mtl_stream &axis_m_3 = stream_3_4;
    mtl_stream &axis_m_4 = stream_4_5;
    mtl_stream &axis_m_5 = stream_5_6;
    mtl_stream &axis_m_6 = stream_6_7;
    mtl_stream &axis_m_7 = stream_7_0;

    // read action config:
    act_reg.Control.flags = 0x0;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    fprintf(stderr, "ACTION_TYPE:   %08x\nRELEASE_LEVEL: %08x\nRETC:          %04x\n",
        (unsigned int)act_config.action_type,
        (unsigned int)act_config.release_level,
        (unsigned int)act_reg.Control.Retc);


    // test action functions:

    fprintf(stderr, "// MAP slot 2 1000:7,2500:3,1700:8\n");
    uint64_t * job_mem = (uint64_t *)host_gmem;
    uint32_t * job_mem_h = (uint32_t *)host_gmem;
    uint8_t * job_mem_b = (uint8_t *)host_gmem;
    job_mem_b[0] = 2; // slot
    job_mem[1] = true; // map
    job_mem[2] = htobe64(3); // extent_count

    job_mem[8]  = htobe64(1000); // ext0.begin
    job_mem[9]  = htobe64(7);    // ext0.count
    job_mem[10] = htobe64(2500); // ext1.begin
    job_mem[11] = htobe64(3);    // ext1.count
    job_mem[12] = htobe64(1700); // ext2.begin
    job_mem[13] = htobe64(8);    // ext2.count

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_MAP;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// MAP slot 7 1200:8,1500:24\n");
    job_mem_b[0] = 7; // slot
    job_mem[1] = true; // map
    job_mem[2] = htobe64(2); // extent_count

    job_mem[8]  = htobe64(1200); // ext0.begin
    job_mem[9]  = htobe64(8);    // ext0.count
    job_mem[10] = htobe64(1500); // ext1.begin
    job_mem[11] = htobe64(24);   // ext1.count

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_MAP;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// QUERY slot 2, lblock 9\n");

    job_mem_b[0] = 2; // slot
    job_mem_b[1] = true; // query_mapping
    job_mem_b[2] = true; // query_state
    job_mem[1] = htobe64(9); // lblock

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_QUERY;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);


    fprintf(stderr, "// ACCESS slot 2, write 6K @ offset 3K\n");
    job_mem_b[0] = 2; // slot
    job_mem_b[1] = true; // write
    job_mem[1] = htobe64(128); // buffer beginning at mem[2]
    job_mem[2] = htobe64(3072); // 3K offset
    job_mem[3] = htobe64(6144); // 6K length

    snap_membus_t data_line = 0;
    mtl_set64(data_line, 0, 0xda7a111100000001ull);
    mtl_set64(data_line, 8, 0xda7a222200000002ull);
    mtl_set64(data_line, 16, 0xda7a333300000003ull);
    mtl_set64(data_line, 24, 0xda7a444400000004ull);
    mtl_set64(data_line, 32, 0xda7a555500000005ull);
    mtl_set64(data_line, 40, 0xda7a666600000006ull);
    mtl_set64(data_line, 48, 0xda7a777700000007ull);
    mtl_set64(data_line, 56, 0xda7a888800000008ull);
    for (int i = 0; i < 96; ++i) {
        host_gmem[2+i] = data_line;
    }

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_ACCESS;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// ACCESS slot 2, read 100 @ offset 3077\n");
    job_mem_b[0] = 2; // slot
    job_mem_b[1] = false; // read
    job_mem[1] = htobe64(7168); // buffer beginning at mem[112]
    job_mem[2] = htobe64(3077); // 3077 offset
    job_mem[3] = htobe64(100); // 100 length

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_ACCESS;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);
    printf("-> %x\n\n", (unsigned int)act_reg.Control.Retc);

    fprintf(stderr, "// CONFIGURE STREAMS\n");
    uint64_t enable = 0;
    enable |= (1 << 0);
    enable |= (1 << 1);
    // enable |= (1 << 2);
    // enable |= (1 << 3);
    enable |= (1 << 4);
    enable |= (1 << 5);
    enable |= (1 << 6);
    enable |= (1 << 7);
    enable |= (1 << 8);
    enable |= (1 << 9);

    job_mem[0] = htobe64(enable);

    uint32_t *master_source = job_mem_h + 2;
    master_source[0] = htobe32(7);
    master_source[1] = htobe32(0);
    master_source[2] = htobe32(1);
    master_source[3] = htobe32(2);
    master_source[4] = htobe32(3);
    master_source[5] = htobe32(4);
    master_source[6] = htobe32(5);
    master_source[7] = htobe32(6);

    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_CONFIGURE_STREAMS;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    fprintf(stderr, "// OP MEM SET READ BUFFER\n");
    job_mem[0] = htobe64(0x80);
    job_mem[1] = htobe64(68);
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_OP_MEM_SET_READ_BUFFER;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    fprintf(stderr, "// OP MEM SET WRITE BUFFER\n");
    job_mem[0] = htobe64(0x100);
    job_mem[1] = htobe64(126);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_OP_MEM_SET_WRITE_BUFFER;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    fprintf(stderr, "// OP CHANGE CASE SET MODE\n");
    job_mem[0] = htobe64(0);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_OP_CHANGE_CASE_SET_MODE;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    fprintf(stderr, "// RUN OPERATORS\n");
    // Fill the memory with 'c' and 'd' characters
    memset(&job_mem_b[0x80], 0, 1024);
    memset(&job_mem_b[0x80], 'c', 64);
    memset(&job_mem_b[0x80 + 64], 'd', 64);

    // Dummy output data
    memset(&job_mem_b[0x100], 'e', 128);
    act_reg.Control.flags = 0x1;
    act_reg.Data.job_address = 0;
    act_reg.Data.job_type = MTL_JOB_RUN_OPERATORS;
    hls_action(host_gmem, host_gmem,
#ifdef NVME_ENABLED
        NULL,
#endif
        axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);

    *(char *)(job_mem_b + 0x100 + 128) = '\0';
    fprintf(stderr, "Result is : %s\n", (char *)(job_mem_b + 0x100));

    fprintf(stderr, "// WRITEBACK \n");
        job_mem[0] = htobe64(0);
        job_mem[1] = htobe64(0);
        job_mem[2] = htobe64(0x80);
        job_mem[3] = htobe64(1);
        act_reg.Data.job_address = 0;
        act_reg.Data.job_type = MTL_JOB_WRITEBACK;
        hls_action(host_gmem, host_gmem,
    #ifdef NVME_ENABLED
            NULL,
    #endif
            axis_s_0, axis_s_1, axis_s_2, axis_s_3, axis_s_4, axis_s_5, axis_s_6, axis_s_7, axis_m_0, axis_m_1, axis_m_2, axis_m_3, axis_m_4, axis_m_5, axis_m_6, axis_m_7, switch_ctrl, NULL, &act_reg, &act_config);


    return 0;
}

#endif /* NO_SYNTH */
