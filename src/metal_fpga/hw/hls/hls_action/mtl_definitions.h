#pragma once

# include "hls_definitions.h"

namespace metal {
namespace fpga {

// Action return code
typedef snapu32_t mtl_retc_t;

// One bit bool type
typedef ap_uint<1> mtl_bool_t;
#define MTL_TRUE 0x1
#define MTL_FALSE 0x0

// Assumptions about BRAM sizes
#define MTL_BRAM_BIT_OFFSET_W 15
#define MTL_BRAM_BITS (0x1<<MTL_BRAM_BIT_OFFSET_W)
#define MTL_BRAM_BYTE_OFFSET_W (MTL_BRAM_BIT_OFFSET_W-4)
#define MTL_BRAM_BYTES (0x1<<MTL_BRAM_BYTE_OFFSET_W)

// A file extent consisting of a begin physical block number
// and a block count
typedef struct mtl_extent {
    snapu64_t block_begin;
    snapu64_t block_count;
} mtl_extent_t;
#define MTL_EXTENT_BYTE_OFFSET_W 4
#define MTL_EXTENT_BYTES (0x1 << MTL_EXTENT_BYTE_OFFSET_W)

// Address into the internal extent lists
#define MTL_EXTENT_COUNT_W 9
typedef ap_uint<MTL_EXTENT_COUNT_W> mtl_extent_offset_t;
typedef ap_uint<MTL_EXTENT_COUNT_W+1> mtl_extent_count_t;

// Address into a memory line with extents
#define MTL_EXTENTS_PER_LINE_W (ADDR_RIGHT_SHIFT - MTL_EXTENT_BYTE_OFFSET_W)
#define MTL_EXTENTS_PER_LINE (0x1 << MTL_EXTENTS_PER_LINE_W)
typedef ap_uint<MTL_EXTENTS_PER_LINE_W> mtl_line_extent_offset_t;
typedef ap_uint<MTL_EXTENTS_PER_LINE_W+1> mtl_line_extent_count_t;

/* // Address into the internal slot list */
/* #define MTL_SLOT_OFFSET_W 4 */
/* #define MTL_SLOT_COUNT (0x1 << MTL_SLOT_OFFSET_W) */
/* typedef ap_uint<MTL_SLOT_OFFSET_W> mtl_slot_offset_t; */
/* typedef ap_uint<MTL_SLOT_OFFSET_W+1> mtl_slot_count_t; */

}  // namespace fpga
}  // namespace metal
