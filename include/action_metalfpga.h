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
    uint8_t slot;
    uint8_t flags; // TODO-lw define Flag masks and positions
    uint16_t extent_count;
    mf_extent_t extents[6];
} mf_func_filemap_job_t;

#define MF_FUNC_EXTENTOP 1
typedef struct mf_func_extentop_job {
    uint8_t operation;
    uint8_t flags; // TODO-lw define Flag masks and positions
    uint8_t _fill[2];
    uint64_t host_buffer;
    mf_extent_t src_extent;
    mf_extent_t dst_extent;
} mf_func_extentop_job_t;

#define MF_FUNC_BUFMAP 2
typedef struct mf_func_bufmap_job {
// TODO-lw define!
} mf_func_bufmap_job_t;

// Blowfish Configuration PATTERN.
// This must match with DATA structure in hls_blowfish/kernel.cpp
// Job description should start with the list of addresses.
typedef struct metalfpga_job {
    uint8_t function;
    uint8_t _fill[3];
    union {
        mf_func_filemap_job_t filemap;
        mf_func_extentop_job_t extentop;
        mf_func_extentop_job_t bufmap;
    } fspec;
} metalfpga_job_t;

#ifdef __cplusplus
}
#endif
#endif	/* __ACTION_METALFPGA_H__ */
