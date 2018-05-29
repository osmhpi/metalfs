#ifndef __MTL_JOBSTRUCT_H__
#define __MTL_JOBSTRUCT_H__

#include "mtl_definitions.h"

typedef struct mtl_job_map {
    snapu64_t slot;
    mtl_bool_t map_else_unmap;
    mtl_extent_count_t extent_count;
    snapu64_t extent_address;
} mtl_job_map_t;

typedef struct mtl_job_fileop {
    snapu64_t slot;
    snapu64_t file_offset;
    snapu64_t dram_offset;
    snapu64_t length;
} mtl_job_fileop_t;

// typedef struct mtl_job_query {
//     mtl_slot_offset_t slot;
//     mtl_bool_t query_mapping;
//     mtl_bool_t query_state;
//     mtl_bool_t is_open;
//     mtl_bool_t is_active;
//     snapu64_t lblock_to_pblock;
//     mtl_extent_count_t extent_count;
//     snapu64_t block_count;
//     snapu64_t current_lblock;
//     snapu64_t current_pblock;
// } mtl_job_query_t;

// typedef struct mtl_job_access {
//     mtl_slot_offset_t slot;
//     mtl_bool_t write_else_read;
//     snapu64_t buffer_address;
//     snapu64_t file_byte_offset;
//     snapu64_t file_byte_count;
// } mtl_job_access_t;


mtl_job_map_t mtl_read_job_map(snap_membus_t * mem, snapu64_t address);

// mtl_job_query_t mtl_read_job_query(snap_membus_t * mem, snapu64_t address);
// void mtl_write_job_query(snap_membus_t * mem, snapu64_t address, mtl_job_query_t query_job);

// mtl_job_access_t mtl_read_job_access(snap_membus_t * mem, snapu64_t address);

mtl_job_fileop_t mtl_read_job_fileop(snap_membus_t * mem, snapu64_t address);

#endif // __MTL_JOBSTRUCT_H__
