#include "mf_jobstruct.h"

#include "hls_globalmem.h"

#include "mf_definitions.h"
#include "mf_endian.h"


mf_job_map_t mf_read_job_map(snap_membus_t * mem, snapu64_t address)
{
    snap_membus_t line = mem[MFB_ADDRESS(address)];
    mf_job_map_t map_job;
    map_job.slot = mf_get64<0>(line);
    map_job.map_else_unmap = (mf_get64<8>(line) == 0) ? MF_FALSE : MF_TRUE;
    map_job.extent_count = mf_get64<16>(line);
    map_job.extent_address = address + MFB_INCREMENT;
    return map_job;
}


// mf_job_query_t mf_read_job_query(snap_membus_t * mem, snapu64_t address)
// {
//     snap_membus_t line = mem[MFB_ADDRESS(address)];
//     mf_job_query_t query_job;
//     query_job.slot = mf_get8(line, 0);
//     query_job.query_mapping = (mf_get8(line, 1) == 0)? MF_FALSE : MF_TRUE;
//     query_job.query_state = (mf_get8(line, 2) == 0)? MF_FALSE : MF_TRUE;
//     query_job.is_open = MF_FALSE;
//     query_job.is_active = MF_FALSE;
//     query_job.lblock_to_pblock = mf_get64<8>(line);
//     query_job.extent_count = 0;
//     query_job.block_count = 0;
//     query_job.current_lblock = 0;
//     query_job.current_pblock = 0;
//     return query_job;
// }

// void mf_write_job_query(snap_membus_t * mem, snapu64_t address, mf_job_query_t query_job)
// {
//     snap_membus_t line = 0;
//     mf_set8(line, 0, query_job.slot);
//     mf_set8(line, 1, query_job.query_mapping? 0xff : 0x00);
//     mf_set8(line, 2, query_job.query_state? 0xff : 0x00);
//     mf_set8(line, 3, query_job.is_open? 0xff : 0x00);
//     mf_set8(line, 4, query_job.is_active? 0xff : 0x00);
//     mf_set64(line, 8, query_job.lblock_to_pblock);
//     mf_set64(line, 16, query_job.extent_count);
//     mf_set64(line, 24, query_job.block_count);
//     mf_set64(line, 32, query_job.current_lblock);
//     mf_set64(line, 40, query_job.current_pblock);
//     mem[MFB_ADDRESS(address)] = line;
// }


// mf_job_access_t mf_read_job_access(snap_membus_t * mem, snapu64_t address)
// {
//     snap_membus_t line = mem[MFB_ADDRESS(address)];
//     mf_job_access_t access_job;
//     access_job.slot = mf_get8(line, 0);
//     access_job.write_else_read = (mf_get8(line, 1) == 0)? MF_FALSE : MF_TRUE;
//     access_job.buffer_address = mf_get64<8>(line);
//     access_job.file_byte_offset = mf_get64<16>(line);
//     access_job.file_byte_count = mf_get64<24>(line);
//     return access_job;
// }


mf_job_fileop_t mf_read_job_fileop(snap_membus_t * mem, snapu64_t address)
{
    snap_membus_t line = mem[MFB_ADDRESS(address)];
    mf_job_fileop_t fileop_job;
    fileop_job.slot = mf_get64<0>(line);
    fileop_job.file_offset = mf_get64<8>(line);
    fileop_job.dram_offset = mf_get64<16>(line);
    fileop_job.length = mf_get64<24>(line);
    return fileop_job;
}

