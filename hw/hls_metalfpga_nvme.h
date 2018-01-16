#include "action_metalfpga.H"

extern mf_buffer_t mf_file_buffers[MF_SLOT_COUNT];

mf_bool_t mf_file_seek(mf_slot_offset_t slot, snapu64_t lblock, mf_bool_t dirty);

mf_bool_t mf_file_next(mf_slot_offset_t slot, mf_bool_t dirty);

mf_bool_t mf_file_flush(mf_slot_offset_t slot);


snapu64_t mf_file_get_lblock(mf_slot_offset_t slot);

snapu64_t mf_file_get_pblock(mf_slot_offset_t slot);

snapu64_t mf_file_at_end(mf_slot_offset_t slot);
