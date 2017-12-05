#ifndef __ACTION_HLS_METALFPGA_H__
#define __ACTION_HLS_METALFPGA_H__

#include <hls_snap.H>
#include <action_metalfpga.h> /*MetalFPGA job definition*/

//---------------------------------------------------------------------
typedef struct {
    CONTROL Control;    /*  16 bytes */
    fs_job_t Data;      /* up to 108 bytes */
    uint8_t padding[SNAP_HLS_JOBSIZE - sizeof(metalfpga_job_t)];
} action_reg;

#endif  /* __ACTION_HLS_METALFPGA_H__ */
