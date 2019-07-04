#pragma once

#include <hls_snap.H>
#include <snap_action_metal.h>

namespace metal {
namespace fpga {

typedef struct {
    CONTROL Control;      /* ==  16 bytes */
    Job Data;             /* <= 108 bytes */
    uint8_t padding[SNAP_HLS_JOBSIZE - sizeof(Job)];
} action_reg;

}  // namespace fpga
}  // namespace metal