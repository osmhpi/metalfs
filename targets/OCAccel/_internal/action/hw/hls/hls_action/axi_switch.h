#pragma once

#include <hls_snap_1024.H>

namespace metal {
namespace fpga {

void switch_set_mapping(snapu32_t *switch_ctrl, snapu32_t data_in, snapu32_t data_out);
void switch_disable_output(snapu32_t *switch_ctrl, snapu32_t data_out);
void switch_commit(snapu32_t *switch_ctrl);

}  // namespace fpga
}  // namespace metal
