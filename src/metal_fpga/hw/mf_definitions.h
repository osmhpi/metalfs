#ifndef __MF_DEFINITIONS_H__
#define __MF_DEFINITIONS_H__

# include "hls_definitions.h"

// Action return code
typedef snapu32_t mf_retc_t;

// One bit bool type
typedef ap_uint<1> mf_bool_t;
#define MF_TRUE 0x1
#define MF_FALSE 0x0

// Assumptions about BRAM sizes
#define MF_BRAM_BIT_OFFSET_W 15
#define MF_BRAM_BITS (0x1<<MF_BRAM_BIT_OFFSET_W)
#define MF_BRAM_BYTE_OFFSET_W (MF_BRAM_BIT_OFFSET_W-4)
#define MF_BRAM_BYTES (0x1<<MF_BRAM_BYTE_OFFSET_W)

// A file extent consisting of a begin physical block number
// and a block count
typedef struct mf_extent {
    snapu64_t block_begin;
    snapu64_t block_count;
} mf_extent_t;
#define MF_EXTENT_BYTE_OFFSET_W 4
#define MF_EXTENT_BYTES (0x1 << MF_EXTENT_BYTE_OFFSET_W)

// Address into the internal extent lists
#define MF_EXTENT_COUNT_W 9
#define MF_EXTENT_COUNT (0x1 << MF_EXTENT_COUNT_W)
typedef ap_uint<MF_EXTENT_OFFSET_W> mf_extent_offset_t;
typedef ap_uint<MF_EXTENT_OFFSET_W+1> mf_extent_count_t;

// Address into a memory line with extents
#define MF_EXTENTS_PER_LINE_W (ADDR_RIGHT_SHIFT - MF_EXTENT_BYTE_OFFSET_W)
#define MF_EXTENTS_PER_LINE (0x1 << MF_EXTENTS_PER_LINE_W)
typedef ap_uint<MF_EXTENTS_PER_LINE_W> mf_line_extent_offset_t;
typedef ap_uint<MF_EXTENTS_PER_LINE_W+1> mf_line_extent_count_t;

/* // Address into the internal slot list */
/* #define MF_SLOT_OFFSET_W 4 */
/* #define MF_SLOT_COUNT (0x1 << MF_SLOT_OFFSET_W) */
/* typedef ap_uint<MF_SLOT_OFFSET_W> mf_slot_offset_t; */
/* typedef ap_uint<MF_SLOT_OFFSET_W+1> mf_slot_count_t; */


#endif // __MF_DEFINITIONS_H__
