#ifndef __ACTION_METALFPGA_H__
#define __ACTION_METALFPGA_H__

/*
 * TODO: licensing, implemented according to IBM snap examples
 */
#include <snap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define METALFPGA_ACTION_TYPE 0x00000216

typedef struct mf_extent {
    uint64_t block_begin;
    uint64_t block_count;
} mf_extent_t;

#define MF_FUNC_FILEMAP 0
typedef struct mf_func_filemap_job {
    uint8_t slot : 8;
    bool map : 1;
    bool indirect : 1;
    uint16_t extent_count : 16;
    union
    {
        mf_extent_t direct[5];
        uint64_t indirect_address;
    } extents;
} mf_func_filemap_job_t;

#define MF_FUNC_FILEQUERY 1
typedef struct mf_func_filequery_job {
    uint8_t slot : 8;
    bool query_mapping : 1;
    bool query_slot_state : 1;
    uint64_t lblock_to_pblock;
    struct {
        bool open : 1;
        bool active : 1;
        uint16_t extent_count;
        uint64_t block_count;
        uint64_t current_lblock;
        uint64_t current_pblock;
    } slot_state;
} mf_func_filequery_job_t;

#define MF_FUNC_ACCESS 2
typedef struct mf_func_access_job {
    uint8_t operation;
    uint8_t flags; // TODO-lw define Flag masks and positions
    uint64_t host_buffer;
    uint64_t file_offset;
    uint64_t byte_count;
} mf_func_access_job_t;

typedef struct metalfpga_job {
    uint8_t function;
    union {
        mf_func_filemap_job_t filemap;
        mf_func_filequery_job_t filequery;
        mf_func_access_job_t access;
    } jspec;
} metalfpga_job_t;

#ifdef __cplusplus
}
#endif
#endif	/* __ACTION_METALFPGA_H__ */
