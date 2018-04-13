#ifndef __MF_FILE_H__
#define __MF_FILE_H__

#include "mf_definitions.h"

#define MF_FILEBLOCK_BYTES_W MF_BRAM_BYTE_OFFSET_W
#define MF_FILEBLOCK_BYTES (0x1<<MF_BLOCK_BYTE_OFFSET_W)
typedef ap_uint<8> mf_filebuf_t[MF_FILEBLOCK_BYTES];
typedef ap_uint<MF_FILEBLOCK_BYTES_W> mf_filebuf_offset_t;
typedef ap_uint<MF_FILEBLOCK_BYTES_W+1> mf_filebuf_count_t;
#define MF_FILEBLOCK_NUMBER(BYTE_OFFSET) (BYTE_OFFSET>>MF_FILEBLOCK_BYTES_W)
#define MF_FILEBLOCK_OFFSET(BYTE_OFFSET) (BYTE_OFFSET&MF_MASK(MF_FILEBLOCK_BYTES_W,0))

typedef struct {
    mf_bool_t is_open;
    mf_bool_t is_active;
    snapu16_t extent_count;
    snapu64_t block_count;
    snapu64_t current_pblock;
    snapu64_t current_lblock;
    snapu64_t block_buffer_address;
    snapu64_t extents_begin[MF_EXTENT_COUNT];
    snapu64_t extents_count[MF_EXTENT_COUNT];
    snapu64_t extents_lastblock[MF_EXTENT_COUNT];
} mf_fileslot_t;

mf_bool_t mf_file_open(mf_fileslot_t & slot,
                       snapu64_t block_buffer_address,
                       mf_extent_count_t extent_count,
                       snapu64_t extent_address,
                       snap_membus_t * mem);

mf_bool_t mf_file_close(mf_fileslot_t & slot);


mf_bool_t mf_file_is_open(mf_fileslot_t & slot);

mf_bool_t mf_file_is_active(mf_fileslot_t & slot);
 
mf_bool_t mf_file_at_end(mf_fileslot_t & slot);

snapu64_t mf_file_get_lblock(mf_fileslot_t & slot);

snapu64_t mf_file_get_pblock(mf_fileslot_t & slot);

snapu16_t mf_file_get_extent_count(mf_fileslot_t & slot);

snapu64_t mf_file_get_block_count(mf_fileslot_t & slot);


snapu64_t mf_file_map_pblock(mf_fileslot_t & slot,
                             snapu64_t lblock);


mf_bool_t mf_file_seek(mf_fileslot_t & slot,
                       mf_filebuf_t & buffer,
                       snap_membus_t * ddr,
                       snapu64_t lblock,
                       mf_bool_t dirty);

mf_bool_t mf_file_next(mf_fileslot_t & slot,
                       mf_filebuf_t & buffer,
                       snap_membus_t * ddr,
                       mf_bool_t dirty);

mf_bool_t mf_file_flush(mf_fileslot_t & slot,
                        mf_filebuf_t & buffer,
                        snap_membus_t * ddr);

#endif // __MF_FILE_H__
