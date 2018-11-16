#ifndef __AXI_SWITCH_H__
#define __AXI_SWITCH_H__

#include <hls_snap.H>

void switch_set_mapping(snapu32_t *switch_ctrl, snapu32_t data_in, snapu32_t data_out);
void switch_disable_output(snapu32_t *switch_ctrl, snapu32_t data_out);
void switch_commit(snapu32_t *switch_ctrl);

#endif // __AXI_SWITCH_H__
