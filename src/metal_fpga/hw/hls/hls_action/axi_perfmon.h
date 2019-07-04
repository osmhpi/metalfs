#pragma once

#include <hls_snap.H>
#include "mtl_definitions.h"

namespace metal {
namespace fpga {

mtl_retc_t action_configure_perfmon(snapu32_t *perfmon_ctrl, uint8_t stream_id0, uint8_t stream_id1);
mtl_retc_t perfmon_reset(snapu32_t *perfmon_ctrl);
mtl_retc_t perfmon_enable(snapu32_t *perfmon_ctrl);
mtl_retc_t perfmon_disable(snapu32_t *perfmon_ctrl);
mtl_retc_t action_perfmon_read(snap_membus_t *mem, snapu32_t *perfmon_ctrl);

}  // namespace fpga
}  // namespace metal