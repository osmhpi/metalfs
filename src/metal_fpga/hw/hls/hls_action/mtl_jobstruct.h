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

mtl_job_map_t mtl_read_job_map(snap_membus_t * mem, snapu64_t address);
mtl_job_fileop_t mtl_read_job_fileop(snap_membus_t * mem, snapu64_t address);

#endif // __MTL_JOBSTRUCT_H__
