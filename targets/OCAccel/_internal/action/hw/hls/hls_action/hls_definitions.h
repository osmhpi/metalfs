#pragma once

#include <hls_snap_1024.H>

namespace metal {
namespace fpga {

#define MTL_MASK(BHI, BLO) ((0x1<<(BHI+1)) - (0x1<<BLO))

// define missing log2 of MEMDW based on addr shift
// (which is the byte instead of bit index width)
#define MEMDW_W (ADDR_RIGHT_SHIFT_512+3)

#define MFB_ADDRESS(A) (A >> ADDR_RIGHT_SHIFT_512)
#define MFB_WRITE(PTR, A, VAL) PTR[MFB_ADDRESS(A)]=VAL
#define MFB_INCREMENT (0x1 << ADDR_RIGHT_SHIFT_512)

typedef ap_uint<ADDR_RIGHT_SHIFT_512>   mfb_byteoffset_t;
typedef ap_uint<ADDR_RIGHT_SHIFT_512+1> mfb_bytecount_t;
typedef ap_uint<ADDR_RIGHT_SHIFT_512+3> mfb_bitoffset_t;
typedef ap_uint<ADDR_RIGHT_SHIFT_512+4> mfb_bitcount_t;
#define MFB_TOBITOFFSET(BYTEO) (((mfb_bitoffset_t)BYTEO)<<3)

#define MFB_LINE_OFFSET(A) (A & MTL_MASK(ADDR_RIGHT_SHIFT_512, 0))

#define MTL_MASK(BHI, BLO) ((0x1<<(BHI+1)) - (0x1<<BLO))

}  // namespace fpga
}  // namespace metal
