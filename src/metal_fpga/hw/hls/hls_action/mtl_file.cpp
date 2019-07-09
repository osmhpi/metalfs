#include "mtl_file.h"

#include "mtl_endian.h"
#include "snap_action_metal.h"

namespace metal {
namespace fpga {

static mtl_bool_t mtl_nvme_write_burst(snapu32_t *d_nvme,
                 snapu64_t ddr_addr,
                 snapu64_t ssd_lb_addr,
                 snap_bool_t drive_id,
                 snapu32_t num_of_blocks_to_transfer)
{
    short rc;
    snapu32_t status;

    // Set card ddr address
    // ddr_addr <= 4GB, so no high part.
    ((volatile int*)d_nvme)[0] = ddr_addr & 0xFFFFFFFF;
    ((volatile int*)d_nvme)[1] = 0x00000002;

    // Set card ssd address
    ((volatile int*)d_nvme)[2] = ssd_lb_addr & 0xFFFFFFFF;
    ((volatile int*)d_nvme)[3] = (ssd_lb_addr >> 32) & 0xFFFFFFFF;

    // Set number of blocks to transfer
    ((volatile int*)d_nvme)[4] = num_of_blocks_to_transfer;

    // Initiate ssd read
    ((volatile int*)d_nvme)[5] = (drive_id == 0)? 0x11: 0x31;

    rc = 1;

    // Poll the status register until the operation is finished
    while(1)
    {
        if((status = ((volatile int*)d_nvme)[1]))
        {
            if(status & 0x10)
                rc = 1;
            else
                rc = 0;
            break;
        }
    }

    return rc == 1 ? MTL_TRUE : MTL_FALSE;
}

static mtl_bool_t mtl_nvme_read_burst(snapu32_t *d_nvme,
                  snapu64_t ddr_addr,
                  snapu64_t ssd_lb_addr,
                  snap_bool_t drive_id,
                  snapu32_t num_of_blocks_to_transfer)
{
    short rc;
    snapu32_t status;

    // Set card ddr address
    // ddr_addr <= 4GB, so no high part.
    ((volatile int*)d_nvme)[0] = ddr_addr & 0xFFFFFFFF;
    ((volatile int*)d_nvme)[1] = 0x00000002;

    // Set card ssd address
    ((volatile int*)d_nvme)[2] = ssd_lb_addr & 0xFFFFFFFF;
    ((volatile int*)d_nvme)[3] = (ssd_lb_addr >> 32) & 0xFFFFFFFF;

    // Set number of blocks to transfer
    ((volatile int*)d_nvme)[4] = num_of_blocks_to_transfer;

    // Initiate ssd read
    ((volatile int*)d_nvme)[5] = (drive_id == 0) ? 0x10: 0x30;

    rc = 1;

    // Poll the status register until the operation is finished
    while (1)
    {
        if ((status = ((volatile int*)d_nvme)[1]))
        {
            if (status & 0x10)
                rc = 1;
            else
                rc = 0;
            break;
        }
    }

    return rc == 1 ? MTL_TRUE : MTL_FALSE;
}

mtl_bool_t mtl_nvme_transfer_buffer(snapu32_t * nvme_ctrl,
    mtl_extmap_t & map,
    snapu64_t lblock,
    snapu64_t dest,
    snapu64_t length,
    snap_bool_t write) {

    if (lblock + length > mtl_extmap_block_count(map)) {
        return MTL_FALSE;
    }

    for (snapu64_t i = 0; i < length; ++i) {
        #pragma HLS PIPELINE
        mtl_extmap_seek(map, lblock + i);

        auto logical_block_offset = mtl_extmap_pblock(map);
        auto dram_addr = dest + (i * StorageBlockSize);
        auto drive_id = logical_block_offset[0];
        auto physical_block_offset = (logical_block_offset >> 1) * (StorageBlockSize / 512);

        if (write) {
            mtl_nvme_write_burst(nvme_ctrl, dram_addr, physical_block_offset, drive_id, (StorageBlockSize / 512) - 1);
        } else {
            mtl_nvme_read_burst(nvme_ctrl, dram_addr, physical_block_offset, drive_id, (StorageBlockSize / 512) - 1);
        }
    }

    return MTL_TRUE;
}

}  // namespace fpga
}  // namespace metal
