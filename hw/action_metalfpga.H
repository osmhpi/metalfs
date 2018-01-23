#ifndef __ACTION_HLS_METALFPGA_H__
#define __ACTION_HLS_METALFPGA_H__

#include <hls_snap.H>
#include <action_metalfpga.h> /*MetalFPGA job definition*/

// define missing log2 of MEMDW based on addr shift
// (which is the byte instead of bit index width)
#define MEMDW_W (ADDR_RIGHT_SHIFT+3)

#define MFB_ADDRESS(A) (A >> ADDR_RIGHT_SHIFT)
#define MFB_WRITE(PTR, A, VAL) PTR[MFB_ADDRESS(A)]=VAL
#define MFB_INCREMENT (0x1 << ADDR_RIGHT_SHIFT)
typedef ap_uint<ADDR_RIGHT_SHIFT> mfb_byteoffset_t;
typedef ap_uint<ADDR_RIGHT_SHIFT+3> mfb_bitoffset_t;
#define MFB_TOBITOFFSET(BYTEO) (((mfb_bitoffset_t)BYTEO)<<3)

typedef snapu32_t mf_retc_t;

#define MF_MASK(BHI, BLO) ((0x1<<(BHI+1)) - (0x1<<BLO))

typedef ap_uint<1> mf_bool_t;
#define MF_TRUE 0x1
#define MF_FALSE 0x0

#define MF_EXTENT_COUNT_W 9
#define MF_EXTENT_COUNT (0x1 << MF_EXTENT_COUNT_W)
typedef ap_uint<MF_EXTENT_COUNT_W> mf_extent_offset_t;
typedef ap_uint<MF_EXTENT_COUNT_W+1> mf_extent_count_t;

#define MF_EXTENT_BYTES_W 4
#define MF_EXTENT_BYTES (0x1 << MF_EXTENT_BYTES_W)
#define MF_EXTENTS_PER_LINE_W (ADDR_RIGHT_SHIFT - MF_EXTENT_BYTES_W)
#define MF_EXTENTS_PER_LINE (0x1 << MF_EXTENTS_PER_LINE_W)
typedef ap_uint<MF_EXTENTS_PER_LINE_W> mf_line_extent_offset_t;
typedef ap_uint<MF_EXTENTS_PER_LINE_W+1> mf_line_extent_count_t;


#define MF_SLOT_COUNT_W 4
#define MF_SLOT_COUNT (0x1 << MF_SLOT_COUNT_W)
typedef ap_uint<MF_SLOT_COUNT_W> mf_slot_offset_t;
typedef ap_uint<MF_SLOT_COUNT_W+1> mf_slot_count_t;

#define MF_BRAM_BITS_W 15
#define MF_BRAM_BITS (0x1<<MF_BRAM_BITS_W)
#define MF_BUFFER_BYTES_W (MF_BRAM_BITS_W-3)
#define MF_BUFFER_BYTES (0x1<<MF_BUFFER_BYTES_W)
#define MF_BUFFER_LINES_W (MF_BRAM_BITS_W-MEMDW_W)
#define MF_BUFFER_LINES (0x1<<MF_BUFFER_LINES_W)
typedef snap_membus_t mf_buffer_t[MF_BUFFER_LINES];
typedef ap_uint<MF_BUFFER_LINES_W> mf_buffer_line_offset_t;
typedef ap_uint<MF_BUFFER_LINES_W+1> mf_buffer_line_count_t;


//---------------------------------------------------------------------
typedef struct {
    CONTROL Control;    /*  16 bytes */
    metalfpga_job_t Data;      /* up to 108 bytes */
    uint8_t padding[SNAP_HLS_JOBSIZE - sizeof(metalfpga_job_t)];
} action_reg;

#endif  /* __ACTION_HLS_METALFPGA_H__ */
