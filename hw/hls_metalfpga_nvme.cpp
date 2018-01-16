#include "hls_metalfpga_access.h"


#include "action_metalfpga.H"
#include "endianconv.hpp"
#include "hls_metalfpga_map.h"


extern mf_buffer_t mf_file_buffers[MF_SLOT_COUNT];

mf_bool_t mf_file_seek(mf_slot_offset_t slot, snapu64_t lblock, mf_bool_t dirty);

mf_bool_t mf_file_next(mf_slot_offset_t slot, mf_bool_t dirty);

mf_bool_t mf_file_flush(mf_slot_offset_t slot);


snapu64_t mf_file_get_lblock(mf_slot_offset_t slot);

snapu64_t mf_file_get_pblock(mf_slot_offset_t slot);

snapu64_t mf_file_at_end(mf_slot_offset_t slot);

// NVMe interface functions:

/**
 * This function represents a read command to the NVMe Controller and
 * is meant to return only if the command is completed.
 */
static void mf_read_block_dummy(uint64_t ram_block_address, uint64_t nvme_block_address)
{
    
}

/**
 * This function represents a read command to the NVMe Controller and
 * is meant to return only if the command is completed.
 */
static void mf_write_block_dummy(uint64_t ram_block_address, uint64_t nvme_block_address)
{
    
}

