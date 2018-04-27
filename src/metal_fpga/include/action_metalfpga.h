#ifndef __ACTION_METALFPGA_H_HLS__
#define __ACTION_METALFPGA_H_HLS__

/*
 * TODO: licensing, implemented according to IBM snap examples
 */
#include "stdbool.h"
#include <snap_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define METALFPGA_ACTION_TYPE 0x00000216

#define MF_MASK(BHI, BLO) ((0x1<<(BHI+1)) - (0x1<<BLO))

#define MF_JOB_MAP 0
// 64bit words at job_address:
//   word0: slot                | R
//   word1: map_else_unmap      | R
//   word2: extent_count        | R
//   ...
//   word(2n+8): extent n begin | R
//   word(2n+9): extent n count | R

#define MF_JOB_QUERY 1
// 64bit words at job_address:
//   word0:                     |
//     byte0: slot              | R
//     byte1: query_mapping     | R
//     byte2: query_state       | R
//     byte3: is_open           | W  if query_state
//     byte4: is_active         | W  if query_state
//   word1: lblock_to_pblock    | RW if query_mapping
//   word2: extent_count        | W  if query_state
//   word3: block_count         | W  if query_state
//   word4: current_lblock      | W  if query_state
//   word5: current_pblock      | W  if query_state

#define MF_JOB_ACCESS 2
// 64bit words at job_address:
//   word0:                     |
//     byte0: slot              | R
//     byte1: write_else_read   | R
//   word1: buffer_address      | R
//   word2: file_byte_offset    | R
//   word3: file_byte_count     | R

#define MF_JOB_MOUNT 3
// 64bit words at job_address:
//   word0: slot                | R
//   word1: file_offset (blocks)| R
//   word2: dram_offset         | R
//   word3: length (blocks)     | R

#define MF_JOB_WRITEBACK 4
// 64bit words at job_address:
//   word0: slot                | R
//   word1: file_offset (blocks)| R
//   word2: dram_offset         | R
//   word3: length (blocks)     | R

#define MF_JOB_CONFIGURE_STREAMS 5
// 64bit words at job_address:
//   word0: enable_mask         | R
//   word1:                     | R
//     halfword0: stream 0 dest | R
//     halfword1: stream 1 dest | R
//   ...
//   word4:                     | R
//     halfword0: stream 6 dest | R
//     halfword1: stream 7 dest | R

#define MF_JOB_RUN_AFUS 6
// no payload data

#define MF_JOB_AFU_MEM_SET_READ_BUFFER 7
// 64bit words at job_address:
//   word0: buffer_address      | R
//   word1: buffer_length       | R

#define MF_JOB_AFU_MEM_SET_WRITE_BUFFER 8
// 64bit words at job_address:
//   word0: buffer_address      | R
//   word1: buffer_length       | R

#define MF_JOB_AFU_MEM_SET_DRAM_READ_BUFFER 9
// 64bit words at job_address:
//   word0: buffer_address      | R
//   word1: buffer_length       | R

#define MF_JOB_AFU_MEM_SET_DRAM_WRITE_BUFFER 10
// 64bit words at job_address:
//   word0: buffer_address      | R
//   word1: buffer_length       | R

#define MF_JOB_AFU_CHANGE_CASE_SET_MODE 11
// 64bit words at job_address:
//   word0: mode                | R

typedef struct metalfpga_job {
    uint64_t job_address;
    uint64_t job_type;
} metalfpga_job_t;

#ifdef __cplusplus
}
#endif
#endif // __ACTION_METALFPGA_H_HLS__
