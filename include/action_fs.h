#ifndef __ACTION_FS_H__
#define __ACTION_FS_H__

/*
 * TODO: licensing, implemented according to IBM snap examples
 */
#include <snap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FS_ACTION_TYPE 0x00000216


// Blowfish Configuration PATTERN.
// This must match with DATA structure in hls_blowfish/kernel.cpp
// Job description should start with the list of addresses.
typedef struct fs_job {
    //struct snap_addr input_data;
    //uint32_t data_length;
    //uint32_t mode;
} fs_job_t;

#ifdef __cplusplus
}
#endif
#endif	/* __ACTION_FS_H__ */
