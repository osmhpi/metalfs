#include "mtl_file.h"

#include "mtl_endian.h"


mtl_bool_t mtl_file_seek(mtl_extmap_t & map,
                       mtl_filebuf_t & buffer,
                       snap_membus_t * ddr,
                       snapu64_t lblock)
{
    if (!mtl_extmap_seek(map, lblock)) {
        return MTL_FALSE;
    }
    //TODO-lw READ BLOCK map.current_pblock
    return MTL_TRUE;
}

mtl_bool_t mtl_file_next(mtl_extmap_t & map,
                       mtl_filebuf_t & buffer,
                       snap_membus_t * ddr)
{
    if (!mtl_extmap_next(map)) {
        return MTL_FALSE;
    }
    //TODO-lw READ BLOCK map.current_pblock
    return MTL_TRUE;
}

void mtl_file_flush(mtl_extmap_t & slot,
                   mtl_filebuf_t & buffer,
                   snap_membus_t * ddr)
{
    //TODO-lw WRITE BLOCK map.current_pblock
}

// static mtl_bool_t mtl_file_write_buffer(snap_membus_t * mem, mtl_slot_state_t & slot)
// {
//     snap_membus_t line = 0;
//     ap_uint<MTL_BLOCK_BYTE_OFFSET_W - ADDR_RIGHT_SHIFT> i_line = 0;
//     for (mtl_block_count_t i_byte = 0; i_byte < MTL_BLOCK_BYTES; ++i_byte) {
//         line = (line << 8) | slot.buffer[i_byte];
//         if (i_byte % ADDR_RIGHT_SHIFT == ADDR_RIGHT_SHIFT - 1) {
//             mem[i_line++] = line;
//         }
//     }
//     return MTL_TRUE;
// }

// static mtl_bool_t mtl_file_read_buffer(snap_membus_t * mem, mtl_slot_state_t & slot)
// {
//     snap_membus_t line = 0;
//     ap_uint<MTL_BLOCK_BYTE_OFFSET_W - ADDR_RIGHT_SHIFT> i_line = 0;
//     for (mtl_block_count_t i_byte = 0; i_byte < MTL_BLOCK_BYTES; ++i_byte) {
//         if (i_byte % ADDR_RIGHT_SHIFT == 0) {
//             line = mem[i_line++];
//         }
//         slot.buffer[i_byte] = (line >> (MEMDW - 8)) & 0xff;
//         line = (line << 8);
//     }
//     return MTL_TRUE;
// }

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
    //((volatile int*)d_nvme)[1] = (ddr_addr >> 32) & 0xFFFFFFFF;

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
    //((volatile int*)d_nvme)[1] = (ddr_addr >> 32) & 0xFFFFFFFF;

    // Set card ssd addres
    ((volatile int*)d_nvme)[2] = ssd_lb_addr & 0xFFFFFFFF;
    ((volatile int*)d_nvme)[3] = (ssd_lb_addr >> 32) & 0xFFFFFFFF;

    // Set number of blocks to transfer
    ((volatile int*)d_nvme)[4] = num_of_blocks_to_transfer;

    // Initiate ssd read
    ((volatile int*)d_nvme)[5] = (drive_id == 0)? 0x10: 0x30;

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

// static mtl_bool_t mtl_file_store_buffer(snap_membus_t * mem, snapu32_t * nvme_ctrl, mtl_slot_state_t & slot)
// {
//     return mtl_file_write_buffer(mem + MFB_ADDRESS(slot.block_buffer_address), slot) &&
//             mtl_nvme_write_burst(nvme_ctrl, slot.block_buffer_address, slot.current_pblock, false, MTL_BLOCK_BYTES/512);
// }

// static mtl_bool_t mtl_file_load_buffer(snap_membus_t * mem, snapu32_t * nvme_ctrl, mtl_slot_state_t & slot)
// {
//     return mtl_nvme_read_burst(nvme_ctrl, slot.block_buffer_address, slot.current_pblock, false, MTL_BLOCK_BYTES/512) &&
//             mtl_file_read_buffer(mem + MFB_ADDRESS(slot.block_buffer_address), slot);
// }

mtl_bool_t mtl_file_load_buffer(snapu32_t * nvme_ctrl,
    mtl_extmap_t & map,
    snapu64_t lblock,
    snapu64_t dest,
    snapu64_t length) {

    mtl_extmap_seek(map, lblock);
    snapu64_t remaining_blocks = length;
    snapu64_t current_offset = 0;
    snapu64_t end = lblock + length;

    if (lblock + remaining_blocks > mtl_extmap_block_count(map)) {
        return MTL_FALSE;
    }

    while (remaining_blocks > 0) {
        snapu64_t current_extent_mount_length = mtl_extmap_remaining_blocks(map, end);
        mtl_nvme_read_burst(nvme_ctrl,
            dest + (current_offset * MTL_BLOCK_BYTES),
            mtl_extmap_pblock(map) * (MTL_BLOCK_BYTES / 512),
            0,
            current_extent_mount_length * (MTL_BLOCK_BYTES / 512) - 1 // length is zero-based
        );
        current_offset += current_extent_mount_length;
        remaining_blocks -= current_extent_mount_length;

        mtl_extmap_next_extent(map);
    }

    return MTL_TRUE;
}

mtl_bool_t mtl_file_write_buffer(snapu32_t * nvme_ctrl,
    mtl_extmap_t & map,
    snapu64_t lblock,
    snapu64_t dest,
    snapu64_t length) {

    mtl_extmap_seek(map, lblock);
    snapu64_t remaining_blocks = length;
    snapu64_t current_offset = 0;
    snapu64_t end = lblock + length;

    if (lblock + remaining_blocks > mtl_extmap_block_count(map)) {
        return MTL_FALSE;
    }

    while (remaining_blocks > 0) {
        snapu64_t current_extent_mount_length = mtl_extmap_remaining_blocks(map, end);
        mtl_nvme_write_burst(nvme_ctrl,
            dest + (current_offset * MTL_BLOCK_BYTES),
            mtl_extmap_pblock(map) * (MTL_BLOCK_BYTES / 512),
            0,
            current_extent_mount_length * (MTL_BLOCK_BYTES / 512) - 1 // length is zero-based
        );
        current_offset += current_extent_mount_length;
        remaining_blocks -= current_extent_mount_length;

        mtl_extmap_next_extent(map);
    }

    return MTL_TRUE;
}
