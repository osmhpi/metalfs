#ifndef __MTL_FILE_H__
#define __MTL_FILE_H__

#include "mtl_definitions.h"
#include "mtl_extmap.h"

#define MTL_FILEBLOCK_BYTES_W MTL_BRAM_BYTE_OFFSET_W
#define MTL_FILEBLOCK_BYTES (0x1<<MTL_BLOCK_BYTE_OFFSET_W)
typedef ap_uint<8> mtl_filebuf_t[MTL_FILEBLOCK_BYTES];
typedef ap_uint<MTL_FILEBLOCK_BYTES_W> mtl_filebuf_offset_t;
typedef ap_uint<MTL_FILEBLOCK_BYTES_W+1> mtl_filebuf_count_t;
#define MTL_FILEBLOCK_NUMBER(BYTE_OFFSET) (BYTE_OFFSET>>MTL_FILEBLOCK_BYTES_W)
#define MTL_FILEBLOCK_OFFSET(BYTE_OFFSET) (BYTE_OFFSET&MTL_MASK(MTL_FILEBLOCK_BYTES_W,0))


mtl_bool_t mtl_file_seek(mtl_extmap_t & map,
                       mtl_filebuf_t & buffer,
                       snap_membus_t * ddr,
                       snapu64_t lblock);

mtl_bool_t mtl_file_next(mtl_extmap_t & map,
                       mtl_filebuf_t & buffer,
                       snap_membus_t * ddr);

void mtl_file_flush(mtl_extmap_t & map,
                   mtl_filebuf_t & buffer,
                   snap_membus_t * ddr);

mtl_bool_t mtl_file_load_buffer(snapu32_t * nvme_ctrl,
                              mtl_extmap_t & map,
                              snapu64_t lblock,
                              snapu64_t dest,
                              snapu64_t length);

mtl_bool_t mtl_file_write_buffer(snapu32_t * nvme_ctrl,
                               mtl_extmap_t & map,
                               snapu64_t lblock,
                               snapu64_t dest,
                               snapu64_t length);

#endif // __MTL_FILE_H__
