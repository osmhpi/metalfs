#include "action_metalfpga.H"

extern mf_buffer_t mf_file_buffers[MF_SLOT_COUNT];


mf_bool_t mf_file_open_direct(  mf_slot_offset_t slot,
                                mf_extent_count_t extent_count,
                                const mf_extent_t extents[MF_EXTENT_DIRECT_COUNT]);

mf_bool_t mf_file_open_indirect(mf_slot_offset_t slot,
                                mf_extent_count_t extent_count,
                                snapu64_t buffer_address,
                                snap_membus_t * host_mem_in);

mf_bool_t mf_file_close(mf_slot_offset_t slot);


mf_bool_t mf_file_is_open(mf_slot_offset_t slot);

mf_bool_t mf_file_is_active(mf_slot_offset_t slot);
 
mf_bool_t mf_file_at_end(mf_slot_offset_t slot);

snapu64_t mf_file_get_lblock(mf_slot_offset_t slot);

snapu64_t mf_file_get_pblock(mf_slot_offset_t slot);

snapu16_t mf_file_get_extent_count(mf_slot_offset_t slot);

snapu64_t mf_file_get_block_count(mf_slot_offset_t slot);


snapu64_t mf_file_map_pblock(   mf_slot_offset_t slot,
                                snapu64_t lblock);


mf_bool_t mf_file_seek(mf_slot_offset_t slot, snapu64_t lblock, mf_bool_t dirty);

mf_bool_t mf_file_next(mf_slot_offset_t slot, mf_bool_t dirty);

mf_bool_t mf_file_flush(mf_slot_offset_t slot);

