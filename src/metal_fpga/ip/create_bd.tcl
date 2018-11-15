# set action_root [lindex $argv 0]
# set log_dir     $action_root
# set log_file    $log_dir/create_bd.log
# set src_dir 	$aip_dir/action_ip_prj/action_ip_prj.srcs/sources_1/ip


set log_file    $log_dir/create_bd.log

set bd_name     bd_action

create_bd_design $bd_name >> $log_file

create_bd_cell -type ip -vlnv xilinx.com:hls:hls_action:1.0 hls_action_0
# set_property -dict [list \
#     CONFIG.C_M_AXI_HOST_MEM_ENABLE_ID_PORTS {true} \
#     CONFIG.C_M_AXI_HOST_MEM_ENABLE_USER_PORTS {true} \
# ] [get_bd_cells hls_action_0]

make_bd_pins_external  \
    [get_bd_pins hls_action_0/ap_clk] \
    [get_bd_pins hls_action_0/ap_rst_n] \
    [get_bd_pins hls_action_0/interrupt]

set_property name ap_clk [get_bd_ports ap_clk_0]
set_property name ap_rst_n [get_bd_ports ap_rst_n_0]
set_property name interrupt [get_bd_ports interrupt_0]

make_bd_intf_pins_external  \
    [get_bd_intf_pins hls_action_0/s_axi_ctrl_reg]

set_property name s_axi_ctrl_reg [get_bd_intf_ports s_axi_ctrl_reg_0]

# Host Mem Crossbar
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_crossbar:2.1 axi_crossbar_0
set_property name axi_host_mem_crossbar [get_bd_cells axi_crossbar_0]

set_property -dict [list CONFIG.NUM_SI {2} CONFIG.NUM_MI {1}] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.DATA_WIDTH.VALUE_SRC USER CONFIG.ADDR_WIDTH.VALUE_SRC USER] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.ADDR_WIDTH {64} CONFIG.DATA_WIDTH {512}] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.BUSER_WIDTH.VALUE_SRC USER CONFIG.RUSER_WIDTH.VALUE_SRC USER CONFIG.WUSER_WIDTH.VALUE_SRC USER] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.WUSER_WIDTH {1} CONFIG.RUSER_WIDTH {1} CONFIG.BUSER_WIDTH {1}] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.ARUSER_WIDTH.VALUE_SRC USER CONFIG.AWUSER_WIDTH.VALUE_SRC USER] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.AWUSER_WIDTH {8} CONFIG.ARUSER_WIDTH {8}] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.M00_A00_BASE_ADDR.VALUE_SRC USER CONFIG.M00_A00_ADDR_WIDTH.VALUE_SRC USER] [get_bd_cells axi_host_mem_crossbar]
set_property -dict [list CONFIG.M00_A00_BASE_ADDR {0x00000000000} CONFIG.M00_A00_ADDR_WIDTH {64}] [get_bd_cells axi_host_mem_crossbar]\

connect_bd_intf_net [get_bd_intf_pins hls_action_0/m_axi_host_mem] [get_bd_intf_pins axi_host_mem_crossbar/S00_AXI]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_host_mem_crossbar/aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_host_mem_crossbar/aresetn]

make_bd_intf_pins_external  [get_bd_intf_pins axi_host_mem_crossbar/M00_AXI]
set_property name m_axi_host_mem [get_bd_intf_ports M00_AXI_0]

# Optional Stuff

if { ( $::env(DDRI_USED) == "TRUE" ) } {
    set_property -dict [list \
        CONFIG.C_M_AXI_CARD_MEM0_ENABLE_ID_PORTS {true} \
        CONFIG.C_M_AXI_CARD_MEM0_ENABLE_USER_PORTS {true} \
    ] [get_bd_cells hls_action_0]

    make_bd_intf_pins_external  \
        [get_bd_intf_pins hls_action_0/m_axi_card_mem0]

    set_property name m_axi_card_mem0 [get_bd_intf_ports m_axi_card_mem0_0]
}

if { ( $::env(NVME_USED) == "TRUE" ) } {
    set_property -dict [list \
        CONFIG.C_M_AXI_NVME_ENABLE_ID_PORTS {true} \
        CONFIG.C_M_AXI_NVME_ENABLE_USER_PORTS {true} \
    ] [get_bd_cells hls_action_0]

    make_bd_intf_pins_external  \
        [get_bd_intf_pins hls_action_0/m_axi_nvme]

    set_property name m_axi_nvme [get_bd_intf_ports m_axi_nvme_0]
}

# AXI Switch

create_bd_cell -type ip -vlnv xilinx.com:ip:axis_switch:1.1 axis_switch_0
set_property -dict [list CONFIG.NUM_SI {8} CONFIG.NUM_MI {8} CONFIG.ROUTING_MODE {1} CONFIG.DECODER_REG {1}] [get_bd_cells axis_switch_0]

connect_bd_intf_net [get_bd_intf_pins hls_action_0/axis_m_0] [get_bd_intf_pins axis_switch_0/S00_AXIS]
connect_bd_intf_net [get_bd_intf_pins hls_action_0/axis_m_1] [get_bd_intf_pins axis_switch_0/S01_AXIS]
connect_bd_intf_net [get_bd_intf_pins hls_action_0/axis_m_2] [get_bd_intf_pins axis_switch_0/S02_AXIS]
connect_bd_intf_net [get_bd_intf_pins hls_action_0/axis_m_3] [get_bd_intf_pins axis_switch_0/S03_AXIS]
connect_bd_intf_net [get_bd_intf_pins hls_action_0/axis_m_4] [get_bd_intf_pins axis_switch_0/S04_AXIS]
connect_bd_intf_net [get_bd_intf_pins hls_action_0/axis_m_5] [get_bd_intf_pins axis_switch_0/S05_AXIS]
connect_bd_intf_net [get_bd_intf_pins hls_action_0/axis_m_6] [get_bd_intf_pins axis_switch_0/S06_AXIS]
connect_bd_intf_net [get_bd_intf_pins hls_action_0/axis_m_7] [get_bd_intf_pins axis_switch_0/S07_AXIS]

connect_bd_intf_net [get_bd_intf_pins axis_switch_0/M01_AXIS] [get_bd_intf_pins hls_action_0/axis_s_1]
connect_bd_intf_net [get_bd_intf_pins axis_switch_0/M02_AXIS] [get_bd_intf_pins hls_action_0/axis_s_2]
connect_bd_intf_net [get_bd_intf_pins axis_switch_0/M03_AXIS] [get_bd_intf_pins hls_action_0/axis_s_3]
connect_bd_intf_net [get_bd_intf_pins axis_switch_0/M04_AXIS] [get_bd_intf_pins hls_action_0/axis_s_4]
connect_bd_intf_net [get_bd_intf_pins axis_switch_0/M05_AXIS] [get_bd_intf_pins hls_action_0/axis_s_5]
connect_bd_intf_net [get_bd_intf_pins axis_switch_0/M06_AXIS] [get_bd_intf_pins hls_action_0/axis_s_6]
connect_bd_intf_net [get_bd_intf_pins axis_switch_0/M07_AXIS] [get_bd_intf_pins hls_action_0/axis_s_7]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axis_switch_0/aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axis_switch_0/s_axi_ctrl_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axis_switch_0/aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axis_switch_0/s_axi_ctrl_aresetn]

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_protocol_converter:2.1 axi_protocol_convert_0
connect_bd_intf_net [get_bd_intf_pins hls_action_0/m_axi_switch_ctrl_reg] [get_bd_intf_pins axi_protocol_convert_0/S_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_protocol_convert_0/M_AXI] [get_bd_intf_pins axis_switch_0/S_AXI_CTRL]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_protocol_convert_0/aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_protocol_convert_0/aresetn]

# Data Mover

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_datamover:5.1 axi_datamover_0
set_property name axi_datamover [get_bd_cells axi_datamover_0]
set_property -dict [list \
    CONFIG.c_include_mm2s {Omit} \
    CONFIG.c_include_mm2s_stsfifo {false} \
    CONFIG.c_include_s2mm_dre {true} \
    CONFIG.c_s2mm_burst_size {64} \
    CONFIG.c_mm2s_include_sf {false} \
    CONFIG.c_s2mm_include_sf {false} \
    CONFIG.c_enable_mm2s {0} \
    CONFIG.c_addr_width {64} \
    CONFIG.c_m_axi_s2mm_id_width {0} \
] [get_bd_cells axi_datamover]
set_property -dict [list CONFIG.c_m_axi_s2mm_data_width.VALUE_SRC USER CONFIG.c_s_axis_s2mm_tdata_width.VALUE_SRC USER] [get_bd_cells axi_datamover]
set_property -dict [list CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {64} CONFIG.c_s2mm_include_sf {true}] [get_bd_cells axi_datamover]

connect_bd_intf_net [get_bd_intf_pins axi_datamover/M_AXI_S2MM] [get_bd_intf_pins axi_host_mem_crossbar/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axis_switch_0/M00_AXIS] [get_bd_intf_pins axi_datamover/S_AXIS_S2MM]
connect_bd_intf_net [get_bd_intf_pins axi_datamover/M_AXIS_S2MM_STS] [get_bd_intf_pins hls_action_0/s2mm_sts]
connect_bd_intf_net [get_bd_intf_pins axi_datamover/S_AXIS_S2MM_CMD] [get_bd_intf_pins hls_action_0/s2mm_cmd_V_V]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_datamover/m_axi_s2mm_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_datamover/m_axi_s2mm_aresetn]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_datamover/m_axis_s2mm_cmdsts_awclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_datamover/m_axis_s2mm_cmdsts_aresetn]

# AXI Perfmon

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_perf_mon:5.0 axi_perf_mon_0
set_property -dict [list CONFIG.C_HAVE_SAMPLED_METRIC_CNT {0} CONFIG.C_SLOT_7_AXI_PROTOCOL {AXI4S} CONFIG.C_SLOT_6_AXI_PROTOCOL {AXI4S} CONFIG.C_SLOT_5_AXI_PROTOCOL {AXI4S} CONFIG.C_SLOT_4_AXI_PROTOCOL {AXI4S} CONFIG.C_SLOT_3_AXI_PROTOCOL {AXI4S} CONFIG.C_SLOT_2_AXI_PROTOCOL {AXI4S} CONFIG.C_SLOT_1_AXI_PROTOCOL {AXI4S} CONFIG.C_SLOT_0_AXI_PROTOCOL {AXI4S} CONFIG.C_GLOBAL_COUNT_WIDTH {64} CONFIG.C_NUM_OF_COUNTERS {10} CONFIG.C_NUM_MONITOR_SLOTS {8}] [get_bd_cells axi_perf_mon_0]

connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_0_AXIS] [get_bd_intf_pins axis_switch_0/M00_AXIS]
connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_1_AXIS] [get_bd_intf_pins axis_switch_0/M01_AXIS]
connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_2_AXIS] [get_bd_intf_pins axis_switch_0/M02_AXIS]
connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_3_AXIS] [get_bd_intf_pins axis_switch_0/M03_AXIS]
connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_4_AXIS] [get_bd_intf_pins axis_switch_0/M04_AXIS]
connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_5_AXIS] [get_bd_intf_pins axis_switch_0/M05_AXIS]
connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_6_AXIS] [get_bd_intf_pins axis_switch_0/M06_AXIS]
connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_7_AXIS] [get_bd_intf_pins axis_switch_0/M07_AXIS]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/s_axi_aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_0_axis_aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_1_axis_aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_2_axis_aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_3_axis_aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_4_axis_aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_5_axis_aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_6_axis_aclk]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_7_axis_aclk]

connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/s_axi_aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_0_axis_aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_1_axis_aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_2_axis_aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_3_axis_aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_4_axis_aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_5_axis_aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_6_axis_aresetn]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_7_axis_aresetn]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/core_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/core_aresetn]

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_protocol_converter:2.1 axi_protocol_convert_1

connect_bd_intf_net [get_bd_intf_pins hls_action_0/m_axi_perfmon_ctrl_reg] [get_bd_intf_pins axi_protocol_convert_1/S_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_protocol_convert_1/M_AXI] [get_bd_intf_pins axi_perf_mon_0/S_AXI]
connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_protocol_convert_1/aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_protocol_convert_1/aresetn]

assign_bd_address >> $log_file

save_bd_design >> $log_file

set_property synth_checkpoint_mode None [get_files  $src_dir/../bd/$bd_name/$bd_name.bd]
generate_target all                     [get_files  $src_dir/../bd/$bd_name/$bd_name.bd] >> $log_file
