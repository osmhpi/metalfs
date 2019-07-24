set log_file    $log_dir/create_bd.log

set bd_name     bd_action

set stream_bytes [exec jq -r ".stream_bytes" $image_json]
set stream_width [expr $stream_bytes * 8]

create_bd_design $bd_name >> $log_file

create_bd_cell -type ip -vlnv user.org:user:hls_action:1.0 snap_action

make_bd_pins_external  \
    [get_bd_pins snap_action/ap_clk] \
    [get_bd_pins snap_action/ap_rst_n] \
    [get_bd_pins snap_action/interrupt]

set_property name ap_clk [get_bd_ports ap_clk_0]
set_property name ap_rst_n [get_bd_ports ap_rst_n_0]
set_property name interrupt [get_bd_ports interrupt_0]

make_bd_intf_pins_external  \
    [get_bd_intf_pins snap_action/s_axi_ctrl_reg]

set_property name s_axi_ctrl_reg [get_bd_intf_ports s_axi_ctrl_reg_0]

# Host Mem Crossbar
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_crossbar:2.1 axi_host_mem_crossbar

set_property -dict [list CONFIG.NUM_SI {2} CONFIG.NUM_MI {1}] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.DATA_WIDTH.VALUE_SRC USER CONFIG.ADDR_WIDTH.VALUE_SRC USER] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.ADDR_WIDTH {64} CONFIG.DATA_WIDTH {512}] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.BUSER_WIDTH.VALUE_SRC USER CONFIG.RUSER_WIDTH.VALUE_SRC USER CONFIG.WUSER_WIDTH.VALUE_SRC USER] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.WUSER_WIDTH {1} CONFIG.RUSER_WIDTH {1} CONFIG.BUSER_WIDTH {1}] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.ARUSER_WIDTH.VALUE_SRC USER CONFIG.AWUSER_WIDTH.VALUE_SRC USER] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.AWUSER_WIDTH {1} CONFIG.ARUSER_WIDTH {1}] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.M00_A00_BASE_ADDR.VALUE_SRC USER CONFIG.M00_A00_ADDR_WIDTH.VALUE_SRC USER] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.M00_A00_BASE_ADDR {0x00000000000} CONFIG.M00_A00_ADDR_WIDTH {64}] [get_bd_cells axi_host_mem_crossbar]

connect_bd_intf_net [get_bd_intf_pins snap_action/m_axi_host_mem] [get_bd_intf_pins axi_host_mem_crossbar/S00_AXI]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_host_mem_crossbar/aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_host_mem_crossbar/aresetn]

make_bd_intf_pins_external  [get_bd_intf_pins axi_host_mem_crossbar/M00_AXI]
set_property name m_axi_host_mem [get_bd_intf_ports M00_AXI_0]

if { ( $::env(NVME_USED) == "TRUE" ) } {
    create_bd_cell -type ip -vlnv user.org:user:NvmeController:1.0 nvme_arbiter
    connect_bd_net [get_bd_ports ap_clk] [get_bd_pins nvme_arbiter/pi_clk]
    connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins nvme_arbiter/pi_rst_n]
    connect_bd_intf_net [get_bd_intf_pins nvme_arbiter/p_rspRd] [get_bd_intf_pins snap_action/nvme_read_resp_V_V]
    connect_bd_intf_net [get_bd_intf_pins nvme_arbiter/p_rspWr] [get_bd_intf_pins snap_action/nvme_write_resp_V_V]
    connect_bd_intf_net [get_bd_intf_pins nvme_arbiter/p_cmdRd] [get_bd_intf_pins snap_action/nvme_read_cmd_V_V]
    connect_bd_intf_net [get_bd_intf_pins nvme_arbiter/p_cmdWr] [get_bd_intf_pins snap_action/nvme_write_cmd_V_V]
    make_bd_intf_pins_external  [get_bd_intf_pins nvme_arbiter/p_nvme]
    set_property name m_axi_nvme [get_bd_intf_ports p_nvme_0]
}

# Metal Switch

create_bd_cell -type ip -vlnv xilinx.com:ip:axis_switch:1.1 metal_switch
set_property -dict [list CONFIG.NUM_SI {8} CONFIG.NUM_MI {8} CONFIG.ROUTING_MODE {1} CONFIG.DECODER_REG {1}] [get_bd_cells metal_switch]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins metal_switch/aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins metal_switch/s_axi_ctrl_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins metal_switch/aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins metal_switch/s_axi_ctrl_aresetn]

# Data Mover

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_datamover:5.1 axi_datamover_mm2s

set_property -dict [list \
    CONFIG.c_m_axi_mm2s_data_width {512} \
    CONFIG.c_m_axis_mm2s_tdata_width $stream_width \
    CONFIG.c_include_mm2s_dre {true} \
    CONFIG.c_mm2s_burst_size {64} \
    CONFIG.c_mm2s_btt_used {23} \
    CONFIG.c_include_s2mm {Omit} \
    CONFIG.c_include_s2mm_stsfifo {false} \
    CONFIG.c_s2mm_addr_pipe_depth {3} \
    CONFIG.c_s2mm_include_sf {false} \
    CONFIG.c_m_axi_mm2s_id_width {0} \
    CONFIG.c_m_axi_s2mm_awid {1} \
    CONFIG.c_enable_s2mm {0} \
    CONFIG.c_enable_mm2s_adv_sig {0} \
    CONFIG.c_addr_width {64} \
] [get_bd_cells axi_datamover_mm2s]

connect_bd_intf_net [get_bd_intf_pins snap_action/mm2s_cmd_V_V] [get_bd_intf_pins axi_datamover_mm2s/S_AXIS_MM2S_CMD]
connect_bd_intf_net [get_bd_intf_pins axi_datamover_mm2s/M_AXIS_MM2S_STS] [get_bd_intf_pins snap_action/mm2s_sts]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_datamover_mm2s/m_axi_mm2s_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_datamover_mm2s/m_axi_mm2s_aresetn]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_datamover_mm2s/m_axis_mm2s_cmdsts_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_datamover_mm2s/m_axis_mm2s_cmdsts_aresetn]

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_datamover:5.1 axi_datamover_s2mm

set_property -dict [list \
    CONFIG.c_include_mm2s {Omit} \
    CONFIG.c_include_mm2s_stsfifo {false} \
    CONFIG.c_m_axi_s2mm_data_width {512} \
    CONFIG.c_s_axis_s2mm_tdata_width $stream_width \
    CONFIG.c_include_s2mm_dre {true} \
    CONFIG.c_s2mm_burst_size {64} \
    CONFIG.c_s2mm_btt_used {23} \
    CONFIG.c_mm2s_include_sf {false} \
    CONFIG.c_m_axi_s2mm_id_width {0} \
    CONFIG.c_enable_mm2s {0} \
    CONFIG.c_addr_width {64} \
    CONFIG.c_s2mm_support_indet_btt {true} \
    CONFIG.c_s2mm_include_sf {false} \
] [get_bd_cells axi_datamover_s2mm]

connect_bd_intf_net [get_bd_intf_pins axi_datamover_s2mm/M_AXIS_S2MM_STS] [get_bd_intf_pins snap_action/s2mm_sts]
connect_bd_intf_net [get_bd_intf_pins axi_datamover_s2mm/S_AXIS_S2MM_CMD] [get_bd_intf_pins snap_action/s2mm_cmd_V_V]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_datamover_s2mm/m_axi_s2mm_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_datamover_s2mm/m_axi_s2mm_aresetn]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_datamover_s2mm/m_axis_s2mm_cmdsts_awclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_datamover_s2mm/m_axis_s2mm_cmdsts_aresetn]

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 dm_smartconnect

connect_bd_intf_net [get_bd_intf_pins axi_datamover_s2mm/M_AXI_S2MM] [get_bd_intf_pins dm_smartconnect/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_datamover_mm2s/M_AXI_MM2S] [get_bd_intf_pins dm_smartconnect/S01_AXI]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins dm_smartconnect/aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins dm_smartconnect/aresetn]

if { ( $::env(DDRI_USED) == "TRUE" ) } {
    # AXI Crossbar
    create_bd_cell -type ip -vlnv xilinx.com:ip:axi_crossbar:2.1 axi_crossbar
    connect_bd_intf_net [get_bd_intf_pins dm_smartconnect/M00_AXI] [get_bd_intf_pins axi_crossbar/S00_AXI]

    connect_bd_intf_net [get_bd_intf_pins axi_crossbar/M00_AXI] [get_bd_intf_pins axi_host_mem_crossbar/S01_AXI]

    make_bd_intf_pins_external  [get_bd_intf_pins axi_crossbar/M01_AXI]
    set_property name m_axi_card_mem0 [get_bd_intf_ports M01_AXI_0]
    set_property -dict [list CONFIG.ADDR_WIDTH {32} CONFIG.DATA_WIDTH {512}] [get_bd_intf_ports m_axi_card_mem0]

    set_property -dict [list CONFIG.M00_A00_BASE_ADDR.VALUE_SRC USER CONFIG.M01_A00_BASE_ADDR.VALUE_SRC USER CONFIG.M00_A00_ADDR_WIDTH.VALUE_SRC USER CONFIG.M01_A00_ADDR_WIDTH.VALUE_SRC USER CONFIG.ADDR_WIDTH.VALUE_SRC USER] [get_bd_cells axi_crossbar]
    set_property -dict [list CONFIG.ADDR_WIDTH {64} CONFIG.M00_A00_BASE_ADDR {0} CONFIG.M01_A00_BASE_ADDR {1000000000000000000000000000000000000000000000000000000000000000} CONFIG.M00_A00_ADDR_WIDTH {63} CONFIG.M01_A00_ADDR_WIDTH {32}] [get_bd_cells axi_crossbar]

    set_property -dict [list CONFIG.AWUSER_WIDTH.VALUE_SRC USER CONFIG.ARUSER_WIDTH.VALUE_SRC USER CONFIG.WUSER_WIDTH.VALUE_SRC USER CONFIG.RUSER_WIDTH.VALUE_SRC USER CONFIG.BUSER_WIDTH.VALUE_SRC USER] [get_bd_cells axi_crossbar]
    set_property -dict [list CONFIG.AWUSER_WIDTH {1} CONFIG.ARUSER_WIDTH {1} CONFIG.WUSER_WIDTH {1} CONFIG.RUSER_WIDTH {1} CONFIG.BUSER_WIDTH {1}] [get_bd_cells axi_crossbar]

    connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_crossbar/aclk]
    connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_crossbar/aresetn]
} else {
    connect_bd_intf_net [get_bd_intf_pins dm_smartconnect/M00_AXI] [get_bd_intf_pins axi_host_mem_crossbar/S01_AXI]
}

# Protocol converter

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_protocol_converter:2.1 axi_metal_cpc

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_metal_cpc/aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_metal_cpc/aresetn]
connect_bd_intf_net [get_bd_intf_pins snap_action/m_axi_metal_ctrl_V] [get_bd_intf_pins axi_metal_cpc/S_AXI]

# AXI Crossbar

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_crossbar:2.1 axi_metal_ctrl_crossbar
set_property -dict [list CONFIG.NUM_MI {5}] [get_bd_cells axi_metal_ctrl_crossbar]
set_property -dict [list CONFIG.PROTOCOL.VALUE_SRC USER] [get_bd_cells axi_metal_ctrl_crossbar]
set_property -dict [list CONFIG.PROTOCOL {AXI4LITE} CONFIG.CONNECTIVITY_MODE {SASD} CONFIG.R_REGISTER {1} CONFIG.S00_SINGLE_THREAD {1}] [get_bd_cells axi_metal_ctrl_crossbar]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_metal_ctrl_crossbar/aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_metal_ctrl_crossbar/aresetn]

connect_bd_intf_net [get_bd_intf_pins axi_metal_cpc/M_AXI] [get_bd_intf_pins axi_metal_ctrl_crossbar/S00_AXI]

connect_bd_intf_net [get_bd_intf_pins axi_metal_ctrl_crossbar/M02_AXI] [get_bd_intf_pins metal_switch/S_AXI_CTRL]

# Stream Sink

create_bd_cell -type ip -vlnv user.org:user:hls_streamsink:1.0 axi_streamsink
set_property location {3 1279 1513} [get_bd_cells axi_streamsink]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_streamsink/ap_clk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_streamsink/ap_rst_n]

# Stream Generator
create_bd_cell -type ip -vlnv user.org:user:hls_streamgen:1.0 hls_streamgen

connect_bd_intf_net [get_bd_intf_pins axi_metal_ctrl_crossbar/M01_AXI] [get_bd_intf_pins hls_streamgen/s_axi_ctrl]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins hls_streamgen/ap_clk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins hls_streamgen/ap_rst_n]

# Data Selector

create_bd_cell -type ip -vlnv xilinx.com:ip:axis_switch:1.1 data_selector
set_property -dict [list CONFIG.NUM_SI {3} CONFIG.NUM_MI {3} CONFIG.ROUTING_MODE {1} CONFIG.DECODER_REG {1} CONFIG.M00_S02_CONNECTIVITY {0} CONFIG.M01_S00_CONNECTIVITY {0} CONFIG.M01_S01_CONNECTIVITY {0} CONFIG.M02_S00_CONNECTIVITY {0} CONFIG.M02_S01_CONNECTIVITY {0}] [get_bd_cells data_selector]

connect_bd_intf_net [get_bd_intf_pins data_selector/M00_AXIS] [get_bd_intf_pins metal_switch/S00_AXIS]
connect_bd_intf_net [get_bd_intf_pins metal_switch/M00_AXIS] [get_bd_intf_pins data_selector/S02_AXIS]
connect_bd_intf_net [get_bd_intf_pins data_selector/M02_AXIS] [get_bd_intf_pins axi_streamsink/data]
connect_bd_intf_net [get_bd_intf_pins data_selector/M01_AXIS] [get_bd_intf_pins axi_datamover_s2mm/S_AXIS_S2MM]
connect_bd_intf_net [get_bd_intf_pins axi_datamover_mm2s/M_AXIS_MM2S] [get_bd_intf_pins data_selector/S00_AXIS]
connect_bd_intf_net [get_bd_intf_pins hls_streamgen/out_r] [get_bd_intf_pins data_selector/S01_AXIS]
connect_bd_intf_net [get_bd_intf_pins axi_metal_ctrl_crossbar/M03_AXI] [get_bd_intf_pins data_selector/S_AXI_CTRL]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins data_selector/aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins data_selector/aresetn]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins data_selector/s_axi_ctrl_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins data_selector/s_axi_ctrl_aresetn]

create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 interrupt_concat
set_property -dict [list CONFIG.NUM_PORTS 8] [get_bd_cells interrupt_concat]
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 one
connect_bd_net [get_bd_pins one/dout] [get_bd_pins snap_action/interrupt_reg_V_V_TVALID] >> $log_file
connect_bd_net [get_bd_pins interrupt_concat/dout] [get_bd_pins snap_action/interrupt_reg_V_V_TDATA] >> $log_file

# Image Info
set_property source_mgmt_mode All [current_project]
create_bd_cell -type ip -vlnv user.org:user:ImageInfo:1.0 imageinfo
connect_bd_intf_net [get_bd_intf_pins axi_metal_ctrl_crossbar/M04_AXI] [get_bd_intf_pins imageinfo/p_regs]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins imageinfo/p_clk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins imageinfo/p_rst_n]
assign_bd_address [get_bd_addr_segs {imageinfo/p_regs/reg0 }] >> $log_file


# AXI Perfmon

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_perf_mon:5.0 axi_perf_mon_0

set_property -dict [list \
    CONFIG.C_HAVE_SAMPLED_METRIC_CNT {0} \
    CONFIG.C_GLOBAL_COUNT_WIDTH {64} \
    CONFIG.C_NUM_OF_COUNTERS {10}] [get_bd_cells axi_perf_mon_0]

connect_bd_intf_net [get_bd_intf_pins axi_metal_ctrl_crossbar/M00_AXI] [get_bd_intf_pins axi_perf_mon_0/S_AXI]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/s_axi_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/s_axi_aresetn]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/core_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/core_aresetn]

source $aip_dir/instantiate_operators.tcl

# Finalization

assign_bd_address >> $log_file

include_bd_addr_seg [get_bd_addr_segs -excluded snap_action/Data_m_axi_metal_ctrl_V/SEG_axi_perf_mon_0_Reg] >> $log_file
include_bd_addr_seg [get_bd_addr_segs -excluded snap_action/Data_m_axi_metal_ctrl_V/SEG_data_selector_Reg] >> $log_file
include_bd_addr_seg [get_bd_addr_segs -excluded snap_action/Data_m_axi_metal_ctrl_V/SEG_hls_streamgen_Reg] >> $log_file
include_bd_addr_seg [get_bd_addr_segs -excluded snap_action/Data_m_axi_metal_ctrl_V/SEG_metal_switch_Reg] >> $log_file
include_bd_addr_seg [get_bd_addr_segs -excluded snap_action/Data_m_axi_metal_ctrl_V/SEG_imageinfo_reg0] >> $log_file

set_property offset 0x0000000000000000 [get_bd_addr_segs {snap_action/Data_m_axi_host_mem/SEG_m_axi_host_mem_Reg}]
set_property range 16E [get_bd_addr_segs {snap_action/Data_m_axi_host_mem/SEG_m_axi_host_mem_Reg}]

if { ( $::env(DDRI_USED) == "TRUE" ) } {
    set_property offset 0x8000000000000000 [get_bd_addr_segs {axi_datamover_mm2s/Data_MM2S/SEG_m_axi_card_mem0_Reg}]
    set_property range 4G [get_bd_addr_segs {axi_datamover_mm2s/Data_MM2S/SEG_m_axi_card_mem0_Reg}]
    set_property offset 0x0000000000000000 [get_bd_addr_segs {axi_datamover_mm2s/Data_MM2S/SEG_m_axi_host_mem_Reg}]
    set_property range 8E [get_bd_addr_segs {axi_datamover_mm2s/Data_MM2S/SEG_m_axi_host_mem_Reg}]

    set_property offset 0x8000000000000000 [get_bd_addr_segs {axi_datamover_s2mm/Data_S2MM/SEG_m_axi_card_mem0_Reg}]
    set_property range 4G [get_bd_addr_segs {axi_datamover_s2mm/Data_S2MM/SEG_m_axi_card_mem0_Reg}]
    set_property offset 0x0000000000000000 [get_bd_addr_segs {axi_datamover_s2mm/Data_S2MM/SEG_m_axi_host_mem_Reg}]
    set_property range 8E [get_bd_addr_segs {axi_datamover_s2mm/Data_S2MM/SEG_m_axi_host_mem_Reg}]
} else {
    set_property offset 0x0000000000000000 [get_bd_addr_segs {axi_datamover_mm2s/Data_MM2S/SEG_m_axi_host_mem_Reg}]
    set_property range 16E [get_bd_addr_segs {axi_datamover_mm2s/Data_MM2S/SEG_m_axi_host_mem_Reg}]
    set_property offset 0x0000000000000000 [get_bd_addr_segs {axi_datamover_s2mm/Data_S2MM/SEG_m_axi_host_mem_Reg}]
    set_property range 16E [get_bd_addr_segs {axi_datamover_s2mm/Data_S2MM/SEG_m_axi_host_mem_Reg}]
}

save_bd_design >> $log_file

set_property synth_checkpoint_mode None [get_files  $src_dir/../bd/$bd_name/$bd_name.bd]
generate_target all                     [get_files  $src_dir/../bd/$bd_name/$bd_name.bd] >> $log_file
