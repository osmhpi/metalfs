#ifndef __MF_JOBSTRUCT_H__
#define __MF_JOBSTRUCT_H__

#include "mf_definitions.h"

typedef struct mf_job_map {
    mf_slot_offset_t slot;
    mf_bool_t map_else_unmap;
    mf_extent_count_t extent_count;
    snapu64_t extent_address;
} mf_job_map_t;

typedef struct mf_job_query {
    mf_slot_offset_t slot;
    mf_bool_t query_mapping;
    mf_bool_t query_state;
    mf_bool_t is_open;
    mf_bool_t is_active;
    snapu64_t lblock_to_pblock;
    mf_extent_count_t extent_count;
    snapu64_t block_count;
    snapu64_t current_lblock;
    snapu64_t current_pblock;
} mf_job_query_t;

typedef struct mf_job_access {
    mf_slot_offset_t slot;
    mf_bool_t write_else_read;
    snapu64_t buffer_address;
    snapu64_t file_byte_offset;
    snapu64_t file_byte_count;
} mf_job_access_t;


mf_job_map_t mf_read_job_map(snap_membus_t * mem, snapu64_t address);

mf_job_query_t mf_read_job_query(snap_membus_t * mem, snapu64_t address);
void mf_write_job_query(snap_membus_t * mem, snapu64_t address, mf_job_query_t query_job);

mf_job_access_t mf_read_job_access(snap_membus_t * mem, snapu64_t address);

#endif // __MF_JOBSTRUCT_H__
