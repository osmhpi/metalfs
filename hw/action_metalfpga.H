#ifndef __ACTION_HLS_METALFPGA_H__
#define __ACTION_HLS_METALFPGA_H__

#include <hls_snap.H>
#include <action_metalfpga.h> /*MetalFPGA job definition*/

#define MF_EXTENT_COUNT_W 9
#define MF_EXTENT_COUNT (0x1 << MF_EXTENT_COUNT_W)
typedef ap_uint<MF_EXTENT_COUNT_W> mf_extent_offset_t;
typedef ap_uint<MF_EXTENT_COUNT_W+1> mf_extent_count_t;

#define MF_SLOT_COUNT_W 4
#define MF_SLOT_COUNT (0x1 << MF_SLOT_COUNT_W)
typedef ap_uint<MF_SLOT_COUNT_W> mf_slot_offset_t;
typedef ap_uint<MF_SLOT_COUNT_W+1> mf_slot_count_t;


//---------------------------------------------------------------------
typedef struct {
    CONTROL Control;    /*  16 bytes */
    fs_job_t Data;      /* up to 108 bytes */
    uint8_t padding[SNAP_HLS_JOBSIZE - sizeof(metalfpga_job_t)];
} action_reg;

#endif  /* __ACTION_HLS_METALFPGA_H__ */
