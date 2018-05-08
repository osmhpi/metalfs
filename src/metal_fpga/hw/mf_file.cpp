#include "mf_file.h"

#include "mf_endian.h"


mf_bool_t mf_file_seek(mf_extmap_t & map,
                       mf_filebuf_t & buffer,
                       snap_membus_t * ddr,
                       snapu64_t lblock)
{
    if (!mf_extmap_seek(map, lblock)) {
        return MF_FALSE;
    }
    //TODO-lw READ BLOCK map.current_pblock
    return MF_TRUE;
}

mf_bool_t mf_file_next(mf_extmap_t & map,
                       mf_filebuf_t & buffer,
                       snap_membus_t * ddr)
{
    if (!mf_extmap_next(map)) {
        return MF_FALSE;
    }
    //TODO-lw READ BLOCK map.current_pblock
    return MF_TRUE;
}

void mf_file_flush(mf_extmap_t & slot,
                   mf_filebuf_t & buffer,
                   snap_membus_t * ddr)
{
    //TODO-lw WRITE BLOCK map.current_pblock
}

// static mf_bool_t mf_file_write_buffer(snap_membus_t * mem, mf_slot_state_t & slot)
// {
//     snap_membus_t line = 0;
//     ap_uint<MF_BLOCK_BYTE_OFFSET_W - ADDR_RIGHT_SHIFT> i_line = 0;
//     for (mf_block_count_t i_byte = 0; i_byte < MF_BLOCK_BYTES; ++i_byte) {
//         line = (line << 8) | slot.buffer[i_byte];
//         if (i_byte % ADDR_RIGHT_SHIFT == ADDR_RIGHT_SHIFT - 1) {
//             mem[i_line++] = line;
//         }
//     }
//     return MF_TRUE;
// }

// static mf_bool_t mf_file_read_buffer(snap_membus_t * mem, mf_slot_state_t & slot)
// {
//     snap_membus_t line = 0;
//     ap_uint<MF_BLOCK_BYTE_OFFSET_W - ADDR_RIGHT_SHIFT> i_line = 0;
//     for (mf_block_count_t i_byte = 0; i_byte < MF_BLOCK_BYTES; ++i_byte) {
//         if (i_byte % ADDR_RIGHT_SHIFT == 0) {
//             line = mem[i_line++];
//         }
//         slot.buffer[i_byte] = (line >> (MEMDW - 8)) & 0xff;
//         line = (line << 8);
//     }
//     return MF_TRUE;
// }

static mf_bool_t mf_nvme_write_burst(snapu32_t *d_nvme,
                 snapu64_t ddr_addr,
                 snapu64_t ssd_lb_addr,
                 snap_bool_t drive_id,
                 snapu32_t num_of_blocks_to_transfer)
{
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

    // Poll the status register until the operation is finished
    while(1) {
        if((status = ((volatile int*)d_nvme)[1])) {
            if(status & 0x10) {
                return MF_TRUE;
            } else {
                return MF_FALSE;
            }
        }
    }
}

static mf_bool_t mf_nvme_read_burst(snapu32_t *d_nvme,
                  snapu64_t ddr_addr,
                  snapu64_t ssd_lb_addr,
                  snap_bool_t drive_id,
                  snapu32_t num_of_blocks_to_transfer)
{
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

    // Poll the status register until the operation is finished
    while(1) {
        if((status = ((volatile snapu32_t*)d_nvme)[1])) {
            if(status & 0x10) {
                return MF_TRUE;
            } else {
                return MF_FALSE;
            }
        }
    }
}

// static mf_bool_t mf_file_store_buffer(snap_membus_t * mem, snapu32_t * nvme_ctrl, mf_slot_state_t & slot)
// {
//     return mf_file_write_buffer(mem + MFB_ADDRESS(slot.block_buffer_address), slot) &&
//             mf_nvme_write_burst(nvme_ctrl, slot.block_buffer_address, slot.current_pblock, false, MF_BLOCK_BYTES/512);
// }

// static mf_bool_t mf_file_load_buffer(snap_membus_t * mem, snapu32_t * nvme_ctrl, mf_slot_state_t & slot)
// {
//     return mf_nvme_read_burst(nvme_ctrl, slot.block_buffer_address, slot.current_pblock, false, MF_BLOCK_BYTES/512) &&
//             mf_file_read_buffer(mem + MFB_ADDRESS(slot.block_buffer_address), slot);
// }

mf_bool_t mf_file_load_buffer(snapu32_t * nvme_ctrl,
    mf_extmap_t & map,
    snapu64_t lblock,
    snapu64_t dest,
    snapu64_t length) {

    mf_extmap_seek(map, lblock);
    snapu64_t remaining_blocks = length;
    snapu64_t current_offset = 0;
    snapu64_t end = lblock + length;

    if (lblock + remaining_blocks > mf_extmap_block_count(map)) {
        return MF_FALSE;
    }

    while (remaining_blocks > 0) {
        snapu64_t current_extent_mount_length = mf_extmap_remaining_blocks(map, end);
        mf_nvme_read_burst(nvme_ctrl,
            dest + (current_offset * MF_BLOCK_BYTES),
            mf_extmap_pblock(map),
            0,
            current_extent_mount_length * (MF_BLOCK_BYTES / 512)
        );
        current_offset += current_extent_mount_length;
        remaining_blocks -= current_extent_mount_length;

        mf_extmap_next_extent(map);
    }

    return MF_TRUE;
}

mf_bool_t mf_file_write_buffer(snapu32_t * nvme_ctrl,
    mf_extmap_t & map,
    snapu64_t lblock,
    snapu64_t dest,
    snapu64_t length) {

    mf_extmap_seek(map, lblock);
    snapu64_t remaining_blocks = length;
    snapu64_t current_offset = 0;
    snapu64_t end = lblock + length;

    if (lblock + remaining_blocks > mf_extmap_block_count(map)) {
        return MF_FALSE;
    }

    while (remaining_blocks > 0) {
        snapu64_t current_extent_mount_length = mf_extmap_remaining_blocks(map, end);
        mf_nvme_write_burst(nvme_ctrl,
            dest + (current_offset * MF_BLOCK_BYTES),
            mf_extmap_pblock(map),
            0,
            current_extent_mount_length * (MF_BLOCK_BYTES / 512)
        );
        current_offset += current_extent_mount_length;
        remaining_blocks -= current_extent_mount_length;

        mf_extmap_next_extent(map);
    }

    return MF_TRUE;
}
