#ifndef __HLS_DEFINITIONS_H__
#define __HLS_DEFINITIONS_H__

#include <hls_snap.H>

// define missing log2 of MEMDW based on addr shift
// (which is the byte instead of bit index width)
#define MEMDW_W (ADDR_RIGHT_SHIFT+3)

#define MFB_ADDRESS(A) (A >> ADDR_RIGHT_SHIFT)
#define MFB_WRITE(PTR, A, VAL) PTR[MFB_ADDRESS(A)]=VAL
#define MFB_INCREMENT (0x1 << ADDR_RIGHT_SHIFT)

typedef ap_uint<ADDR_RIGHT_SHIFT> mfb_byteoffset_t;
typedef ap_uint<ADDR_RIGHT_SHIFT+3> mfb_bitoffset_t;
#define MFB_TOBITOFFSET(BYTEO) (((mfb_bitoffset_t)BYTEO)<<3)

#define MFB_LINE_OFFSET(A) (A & MF_MASK(ADDR_RIGHT_SHIFT, 0))

#define MF_MASK(BHI, BLO) ((0x1<<(BHI+1)) - (0x1<<BLO))

#endif // __HLS_DEFINITIONS_H__
