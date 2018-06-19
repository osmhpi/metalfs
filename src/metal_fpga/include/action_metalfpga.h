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

#define MTL_MASK(BHI, BLO) ((0x1<<(BHI+1)) - (0x1<<BLO))

#define MTL_JOB_MAP 0
// 64bit words at job_address:
//   word0: slot                | R
//   word1: map_else_unmap      | R
//   word2: extent_count        | R
//   ...
//   word(2n+8): extent n begin | R
//   word(2n+9): extent n count | R

#define MTL_JOB_QUERY 1
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

#define MTL_JOB_ACCESS 2
// 64bit words at job_address:
//   word0:                     |
//     byte0: slot              | R
//     byte1: write_else_read   | R
//   word1: buffer_address      | R
//   word2: file_byte_offset    | R
//   word3: file_byte_count     | R

#define MTL_JOB_MOUNT 3
// 64bit words at job_address:
//   word0: slot                | R
//   word1: file_offset (blocks)| R
//   word2: dram_offset         | R
//   word3: length (blocks)     | R

#define MTL_JOB_WRITEBACK 4
// 64bit words at job_address:
//   word0: slot                | R
//   word1: file_offset (blocks)| R
//   word2: dram_offset         | R
//   word3: length (blocks)     | R

#define MTL_JOB_CONFIGURE_STREAMS 5
// 64bit words at job_address:
//   word0: enable_mask         | R
//   word1:                     | R
//     halfword0: stream 0 dest | R
//     halfword1: stream 1 dest | R
//   ...
//   word4:                     | R
//     halfword0: stream 6 dest | R
//     halfword1: stream 7 dest | R

#define MTL_JOB_RESET_PERFMON 6

#define MTL_JOB_CONFIGURE_PERFMON 7
// 64bit words at job_address:
//   word0: stream id           | R

#define MTL_JOB_READ_PERFMON_COUNTERS 8
// 32bit words at job_address:
//   word0: global clock ctr    | W
//   word1: counter0            | W
//   word2: counter1            | W
//   word3: counter2            | W
//   word4: counter3            | W
//   word5: counter4            | W
//   word6: counter5            | W
//   word7: counter6            | W
//   word8: counter7            | W
//   word9: counter8            | W
//   word10: counter9           | W

#define MTL_JOB_RUN_OPERATORS 9
// no payload data

#define MTL_JOB_OP_MEM_SET_READ_BUFFER 10
// 64bit words at job_address:
//   word0: buffer_address      | R
//   word1: buffer_length       | R

#define MTL_JOB_OP_MEM_SET_WRITE_BUFFER 11
// 64bit words at job_address:
//   word0: buffer_address      | R
//   word1: buffer_length       | R

#define MTL_JOB_OP_MEM_SET_DRAM_READ_BUFFER 12
// 64bit words at job_address:
//   word0: buffer_address      | R
//   word1: buffer_length       | R

#define MTL_JOB_OP_MEM_SET_DRAM_WRITE_BUFFER 13
// 64bit words at job_address:
//   word0: buffer_address      | R
//   word1: buffer_length       | R

#define MTL_JOB_OP_CHANGE_CASE_SET_MODE 14
// 64bit words at job_address:
//   word0: mode                | R

typedef struct metalfpga_job {
    uint64_t job_address;
    uint64_t job_type;
} metalfpga_job_t;

// Pipeline operator identifiers

#define OP_READ_MEM_ENABLE_ID 0
#define OP_WRITE_MEM_ENABLE_ID 1
#define OP_READ_DRAM_ENABLE_ID 2
#define OP_WRITE_DRAM_ENABLE_ID 3
#define OP_PASSTHROUGH_ENABLE_ID 4
#define OP_CHANGE_CASE_ENABLE_ID 5

#define OP_MEM_STREAM_ID 0
#define OP_DRAM_STREAM_ID 1
#define OP_PASSTHROUGH_STREAM_ID 2
#define OP_CHANGE_CASE_STREAM_ID 3

#ifdef __cplusplus
}
#endif
#endif // __ACTION_METALFPGA_H_HLS__
