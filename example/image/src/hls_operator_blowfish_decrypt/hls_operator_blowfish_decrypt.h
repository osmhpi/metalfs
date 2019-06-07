#ifndef __HLS_OPERATOR_BLOWFISH_DECRYPT_H__
#define __HLS_OPERATOR_BLOWFISH_DECRYPT_H__

#include <hls_common/mtl_stream.h>

// BF_INSTANCES: # of parallel bf_encrypt/bf_decrypt instances to be synthesized
#define BF_INST_W 4
#define BF_INST (1 << BF_INST_W)

//
#define BF_BLOCKBITS 64
#define BF_BLOCK_BADR_BITS 3
#define BF_BLOCK_BADR_MASK 0x7
#define BF_HBLOCKBITS 32
#define BF_HBLOCK_BADR_BITS 2
#define BF_HBLOCK_BADR_MASK 0x3
#define BF_KEY_HBLOCK_MIN 1
#define BF_KEY_HBLOCK_MAX 16

// S arrays (BF_INST/2 copies of 4 arrays with 256 entries * 32b)
#define BF_S_CPYCNT_W (BF_INST_W - 1)
#define BF_S_CPYCNT (1 << BF_S_CPYCNT_W)
#define BF_S_ARYCNT_W 2
#define BF_S_ARYCNT (1 << BF_S_ARYCNT_W)
#define BF_S_ENTCNT_W 8
#define BF_S_ENTCNT (1 << BF_S_ENTCNT_W)
#define BF_S_DATA_W 32

// P array (18 entries * 32b)
#define BF_P_ENTCNT_W 5
#define BF_P_ENTCNT 18
#define BF_P_DATA_W 32

// blocks per line (block: 8B / 64b; line 128B / 1024b)
#define BF_BPL_W 4
#define BF_BPL (1 << BF_BPL_W)
#define BF_HBPL_W (BF_BPL_W + 1)
#define BF_HBPL (1 << BF_HBPL_W)

typedef ap_uint<BF_BLOCKBITS> bf_block_t;
typedef ap_uint<BF_HBLOCKBITS> bf_halfBlock_t;

typedef ap_uint<BF_S_DATA_W> bf_S_t[BF_S_CPYCNT][BF_S_ARYCNT][BF_S_ENTCNT];
typedef ap_uint<BF_S_DATA_W> bf_initS_t[BF_S_ARYCNT][BF_S_ENTCNT];
typedef ap_uint<BF_S_CPYCNT_W + 1> bf_SiC_t;
typedef ap_uint<BF_S_ARYCNT_W + 1> bf_SiA_t;
typedef ap_uint<BF_S_ENTCNT_W + 1> bf_SiE_t;

typedef ap_uint<BF_P_DATA_W> bf_P_t[BF_P_ENTCNT];
typedef ap_uint<BF_P_ENTCNT_W> bf_PiE_t;

typedef ap_uint<BF_BPL_W + 1> bf_uiBpL_t;
typedef ap_uint<BF_HBPL_W + 1> bf_uiHBpL_t;

typedef ap_uint<5> bf_iteration_t;

#endif // __HLS_OPERATOR_BLOWFISH_DECRYPT_H__
