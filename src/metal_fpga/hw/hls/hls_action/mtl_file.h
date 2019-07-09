#pragma once

#include "mtl_definitions.h"
#include "mtl_extmap.h"

namespace metal {
namespace fpga {

mtl_bool_t mtl_nvme_transfer_buffer(snapu32_t * nvme_ctrl,
                                    mtl_extmap_t & map,
                                    snapu64_t lblock,
                                    snapu64_t dest,
                                    snapu64_t length,
                                    snap_bool_t write);

}  // namespace fpga
}  // namespace metal
