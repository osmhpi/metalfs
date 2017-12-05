#include <string.h>
#include <iostream>

#include "action_fs.H"

#define HW_RELEASE_LEVEL       0x00000013

using namespace std;


// ------------------------------------------------
// --------------- ACTION FUNCTIONS ---------------
// ------------------------------------------------


static snapu32_t process_action(snap_membus_t * din_gmem,
                                snap_membus_t * dout_gmem,
                                action_reg * action_reg)
{
    /* snapu64_t inAddr; */
    /* snapu64_t outAddr; */
    /* snapu32_t byteCount; */
    /* snapu32_t mode; */

    /* // byte address received need to be aligned with port width */
    /* inAddr  = action_reg->Data.input_data.addr; */
    /* outAddr  = action_reg->Data.output_data.addr; */
    /* byteCount = action_reg->Data.data_length; */
    /* mode = action_reg->Data.mode; */

    snapu32_t retc = SNAP_RETC_SUCCESS;
    /* switch (mode) { */
    /* case MODE_SET_KEY: */
    /*     retc = action_setkey(din_gmem, inAddr, byteCount); */
    /*     break; */
    /* case MODE_ENCRYPT: */
    /*     retc = action_endecrypt(din_gmem, inAddr, dout_gmem, outAddr, */
    /*                 byteCount, 0); */
    /*     break; */
    /* case MODE_DECRYPT: */
    /*     retc = action_endecrypt(din_gmem, inAddr, dout_gmem, outAddr, */
    /*                 byteCount, 1); */
    /*     break; */
    /* default: */
    /*     break; */
    /* } */

    return retc;
}

// This example doesn't use FPGA DDR.
// Need to set Environment Variable "SDRAM_USED=FALSE" before compilation.
void hls_action(snap_membus_t  *din_gmem, snap_membus_t  *dout_gmem,
        action_reg *action_reg, action_RO_config_reg *Action_Config)
{
    // Host Memory AXI Interface
#pragma HLS INTERFACE m_axi port=din_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE m_axi port=dout_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE s_axilite port=din_gmem bundle=ctrl_reg offset=0x030
#pragma HLS INTERFACE s_axilite port=dout_gmem bundle=ctrl_reg offset=0x040

    // Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=Action_Config
#pragma HLS INTERFACE s_axilite port=Action_Config bundle=ctrl_reg  offset=0x010
#pragma HLS DATA_PACK variable=action_reg
#pragma HLS INTERFACE s_axilite port=action_reg bundle=ctrl_reg offset=0x100
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_reg

    /* Required Action Type Detection */
    switch (action_reg->Control.flags) {
        case 0:
        Action_Config->action_type = (snapu32_t)FS_ACTION_TYPE;
        Action_Config->release_level = (snapu32_t)HW_RELEASE_LEVEL;
        action_reg->Control.Retc = (snapu32_t)0xe00f;
        break;
        default:
        action_reg->Control.Retc = process_action(din_gmem, dout_gmem, action_reg);
        break;
    }
}

//-----------------------------------------------------------------------------
//--- TESTBENCH ---------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef NO_SYNTH

int main()
{
    static snap_membus_t din_gmem[1024];
    static snap_membus_t dout_gmem[1024];
    action_reg act_reg;
    action_RO_config_reg act_config;

    // read action config:
    act_reg.Control.flags = 0x0;
    hls_action(din_gmem, dout_gmem, &act_reg, &act_config);
    fprintf(stderr, "ACTION_TYPE:   %08x\nRELEASE_LEVEL: %08x\nRETC:          %04x\n",
        (unsigned int)act_config.action_type,
        (unsigned int)act_config.release_level,
        (unsigned int)act_reg.Control.Retc);


    // test action functions:

    /* fprintf(stderr, "// MODE_SET_KEY ciphertext 16 Byte at 0x100\n"); */
    /* act_reg.Control.flags = 0x1; */
    /* act_reg.Data.input_data.addr = 0; */
    /* act_reg.Data.data_length = 8; */
    /* act_reg.Data.mode = MODE_SET_KEY; */
    /* hls_action(din_gmem, dout_gmem, &act_reg, &act_config); */

    return 0;
}

#endif /* NO_SYNTH */
