#pragma once

#include "stdbool.h"
#include <snap_types.h>

namespace metal {
namespace fpga {

const uint64_t ActionType = 0xFB060001;

enum class JobType : uint64_t {
    Map,
    // 64bit words at job_address:
    //   word0: slot                | R
    //   word1: map_else_unmap      | R
    //   word2: extent_count        | R
    //   ...
    //   word(2n+8): extent n begin | R
    //   word(2n+9): extent n count | R

    Query,
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

    Access,
    // 64bit words at job_address:
    //   word0:                     |
    //     byte0: slot              | R
    //     byte1: write_else_read   | R
    //   word1: buffer_address      | R
    //   word2: file_byte_offset    | R
    //   word3: file_byte_count     | R

    Mount,
    // 64bit words at job_address:
    //   word0: slot                | R
    //   word1: file_offset (blocks)| R
    //   word2: dram_offset         | R
    //   word3: length (blocks)     | R

    Writeback,
    // 64bit words at job_address:
    //   word0: slot                | R
    //   word1: file_offset (blocks)| R
    //   word2: dram_offset         | R
    //   word3: length (blocks)     | R

    ConfigureStreams,
    // 64bit words at job_address:
    //   word0:                     | R
    //     halfword0: stream 0 dest | R
    //     halfword1: stream 1 dest | R
    //   ...
    //   word3:                     | R
    //     halfword0: stream 6 dest | R
    //     halfword1: stream 7 dest | R

    ResetPerfmon,

    ConfigurePerfmon,
    // 64bit words at job_address:
    //   word0: stream id 0         | R
    //   word0: stream id 1         | R

    ReadPerfmonCounters,
    // 64bit words at job_address:
    //   word0: global clock ctr    | W
    // 32bit words at job_address + 8:
    //   word0: counter0            | W
    //   word1: counter1            | W
    //   word2: counter2            | W
    //   word3: counter3            | W
    //   word4: counter4            | W
    //   word5: counter5            | W
    //   word6: counter6            | W
    //   word7: counter7            | W
    //   word8: counter8            | W
    //   word9: counter9            | W

    RunOperators,
    // no indirect payload data
    // 64bit words direct data:
    //   word0: enable mask         | R
    //   word1: perfmon_enable      | R
    //   word2: bytes_written       | W

    SetReadBuffer,
    // 64bit words at job_address:
    //   word0: buffer_address      | R
    //   word1: buffer_length       | R
    //   word2: mode                | R

    SetWriteBuffer,
    // 64bit words at job_address:
    //   word0: buffer_address      | R
    //   word1: buffer_length       | R
    //   word2: mode                | R

    ConfigureOperator
    // 32bit words at job_address:
    //   word0: offset in sizeof(snapu32_t) | R
    //   word1: length in sizeof(snapu32_t) | R
    //   word2: operator id         | R
    //   word3: prepare             | R
    // <...> configuration data
};

struct Job {
    uint64_t job_address;
    JobType job_type;
    uint64_t direct_data[4];
};

#define OP_MEM_MODE_HOST 0
#define OP_MEM_MODE_DRAM 1
#define OP_MEM_MODE_NULL 2
#define OP_MEM_MODE_RANDOM 3

}  // namespace fpga
}  // namespace metal