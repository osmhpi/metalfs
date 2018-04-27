#ifndef __MF_FILE_H__
#define __MF_FILE_H__

#include "mf_definitions.h"
#include "mf_extmap.h"

#define MF_FILEBLOCK_BYTES_W MF_BRAM_BYTE_OFFSET_W
#define MF_FILEBLOCK_BYTES (0x1<<MF_BLOCK_BYTE_OFFSET_W)
typedef ap_uint<8> mf_filebuf_t[MF_FILEBLOCK_BYTES];
typedef ap_uint<MF_FILEBLOCK_BYTES_W> mf_filebuf_offset_t;
typedef ap_uint<MF_FILEBLOCK_BYTES_W+1> mf_filebuf_count_t;
#define MF_FILEBLOCK_NUMBER(BYTE_OFFSET) (BYTE_OFFSET>>MF_FILEBLOCK_BYTES_W)
#define MF_FILEBLOCK_OFFSET(BYTE_OFFSET) (BYTE_OFFSET&MF_MASK(MF_FILEBLOCK_BYTES_W,0))


mf_bool_t mf_file_seek(mf_extmap_t & map,
                       mf_filebuf_t & buffer,
                       snap_membus_t * ddr,
                       snapu64_t lblock);

mf_bool_t mf_file_next(mf_extmap_t & map,
                       mf_filebuf_t & buffer,
                       snap_membus_t * ddr);

void mf_file_flush(mf_extmap_t & map,
                   mf_filebuf_t & buffer,
                   snap_membus_t * ddr);

mf_bool_t mf_file_load_buffer(snapu32_t * nvme_ctrl,
                              mf_extmap_t & map,
                              snapu64_t lblock,
                              snapu64_t dest,
                              snapu64_t length);

mf_bool_t mf_file_write_buffer(snapu32_t * nvme_ctrl,
                               mf_extmap_t & map,
                               snapu64_t lblock,
                               snapu64_t dest,
                               snapu64_t length);

#endif // __MF_FILE_H__
