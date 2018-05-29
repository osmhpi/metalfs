#include "mtl_jobstruct.h"

#include "mtl_definitions.h"
#include "mtl_endian.h"


mtl_job_map_t mtl_read_job_map(snap_membus_t * mem, snapu64_t address)
{
    snap_membus_t line = mem[MFB_ADDRESS(address)];
    mtl_job_map_t map_job;
    map_job.slot = mtl_get64<0>(line);
    map_job.map_else_unmap = (mtl_get64<8>(line) == 0) ? MTL_FALSE : MTL_TRUE;
    map_job.extent_count = mtl_get64<16>(line);
    map_job.extent_address = address + MFB_INCREMENT;
    return map_job;
}


// mtl_job_query_t mtl_read_job_query(snap_membus_t * mem, snapu64_t address)
// {
//     snap_membus_t line = mem[MFB_ADDRESS(address)];
//     mtl_job_query_t query_job;
//     query_job.slot = mtl_get8(line, 0);
//     query_job.query_mapping = (mtl_get8(line, 1) == 0)? MTL_FALSE : MTL_TRUE;
//     query_job.query_state = (mtl_get8(line, 2) == 0)? MTL_FALSE : MTL_TRUE;
//     query_job.is_open = MTL_FALSE;
//     query_job.is_active = MTL_FALSE;
//     query_job.lblock_to_pblock = mtl_get64<8>(line);
//     query_job.extent_count = 0;
//     query_job.block_count = 0;
//     query_job.current_lblock = 0;
//     query_job.current_pblock = 0;
//     return query_job;
// }

// void mtl_write_job_query(snap_membus_t * mem, snapu64_t address, mtl_job_query_t query_job)
// {
//     snap_membus_t line = 0;
//     mtl_set8(line, 0, query_job.slot);
//     mtl_set8(line, 1, query_job.query_mapping? 0xff : 0x00);
//     mtl_set8(line, 2, query_job.query_state? 0xff : 0x00);
//     mtl_set8(line, 3, query_job.is_open? 0xff : 0x00);
//     mtl_set8(line, 4, query_job.is_active? 0xff : 0x00);
//     mtl_set64(line, 8, query_job.lblock_to_pblock);
//     mtl_set64(line, 16, query_job.extent_count);
//     mtl_set64(line, 24, query_job.block_count);
//     mtl_set64(line, 32, query_job.current_lblock);
//     mtl_set64(line, 40, query_job.current_pblock);
//     mem[MFB_ADDRESS(address)] = line;
// }


// mtl_job_access_t mtl_read_job_access(snap_membus_t * mem, snapu64_t address)
// {
//     snap_membus_t line = mem[MFB_ADDRESS(address)];
//     mtl_job_access_t access_job;
//     access_job.slot = mtl_get8(line, 0);
//     access_job.write_else_read = (mtl_get8(line, 1) == 0)? MTL_FALSE : MTL_TRUE;
//     access_job.buffer_address = mtl_get64<8>(line);
//     access_job.file_byte_offset = mtl_get64<16>(line);
//     access_job.file_byte_count = mtl_get64<24>(line);
//     return access_job;
// }


mtl_job_fileop_t mtl_read_job_fileop(snap_membus_t * mem, snapu64_t address)
{
    snap_membus_t line = mem[MFB_ADDRESS(address)];
    mtl_job_fileop_t fileop_job;
    fileop_job.slot = mtl_get64<0>(line);
    fileop_job.file_offset = mtl_get64<8>(line);
    fileop_job.dram_offset = mtl_get64<16>(line);
    fileop_job.length = mtl_get64<24>(line);
    return fileop_job;
}

