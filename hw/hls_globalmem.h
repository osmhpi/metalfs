#ifndef __HLS_GLOBALMEM_H__
#define __HLS_GLOBALMEM_H__

#include <hls_snap.H>


extern snap_membus_t * gmem_host_in;
extern snap_membus_t * gmem_host_out;
extern snap_membus_t * gmem_ddr;


/* // Configure Host Memory AXI Interface */
/* #pragma HLS INTERFACE m_axi port=gmem_host_out bundle=host_mem offset=slave depth=512 */
/* #pragma HLS INTERFACE m_axi port=gmem_host_in bundle=host_mem offset=slave depth=512 */
/* #pragma HLS INTERFACE s_axilite port=gmem_host_out bundle=ctrl_reg offset=0x030 */
/* #pragma HLS INTERFACE s_axilite port=gmem_host_in bundle=ctrl_reg offset=0x040 */

/* // Configure DDR memory Interface */
/* #pragma HLS INTERFACE m_axi port=gmem_ddr bundle=card_mem0 offset=slave depth=512 \ */
/*   max_read_burst_length=64  max_write_burst_length=64 */ 
/* #pragma HLS INTERFACE s_axilite port=gmem_ddr bundle=ctrl_reg offset=0x050 */

#endif // __HLS_GLOBALMEM_H__
