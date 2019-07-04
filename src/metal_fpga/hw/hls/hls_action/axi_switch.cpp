#include "axi_switch.h"

namespace metal {
namespace fpga {

#define AXI_STREAM_SWITCH_MAPPING_CONFIG_OFFSET (0x40 / sizeof(uint32_t))

void switch_set_mapping(snapu32_t *switch_ctrl, snapu32_t data_in, snapu32_t data_out) {
    snapu32_t *switch_mappings = switch_ctrl + AXI_STREAM_SWITCH_MAPPING_CONFIG_OFFSET;

    switch_mappings[data_out] = data_in;
}

void switch_disable_output(snapu32_t *switch_ctrl, snapu32_t data_out) {
    snapu32_t *switch_mappings = switch_ctrl + AXI_STREAM_SWITCH_MAPPING_CONFIG_OFFSET;

    switch_mappings[data_out] = 0x80000000;
}

void switch_commit(snapu32_t *switch_ctrl) {
    switch_ctrl[0] = 0x2;
}

}  // namespace fpga
}  // namespace metal
