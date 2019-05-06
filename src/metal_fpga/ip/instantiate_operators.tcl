set operators [eval dict create [split [exec jq -r ".operators | to_entries | map(.key, .value.source) | join(\" \")" $action_root/hw/image.json]]]

set streams_count [expr [dict size $operators] + 1]

set_property -dict [list CONFIG.NUM_SI $streams_count CONFIG.NUM_MI $streams_count] [get_bd_cells metal_switch]
set_property -dict [list CONFIG.NUM_MI [expr [dict size $operators] + 4]] [get_bd_cells axi_metal_ctrl_crossbar]
set_property -dict [list CONFIG.NUM_PORTS [dict size $operators]] [get_bd_cells interrupt_concat]

set i 1

dict for {id path} $operators {
    set component [file tail $path]
    create_bd_cell -type ip -vlnv xilinx.com:hls:$component:1.0 op_$id

    connect_bd_intf_net [get_bd_intf_pins op_$id/axis_output] [get_bd_intf_pins metal_switch/S0${i}_AXIS]
    connect_bd_intf_net [get_bd_intf_pins metal_switch/M0${i}_AXIS] [get_bd_intf_pins op_$id/axis_input]

    set control_port [expr $i + 3]
    connect_bd_intf_net [get_bd_intf_pins axi_metal_ctrl_crossbar/M0${control_port}_AXI] [get_bd_intf_pins op_$id/s_axi_control]

    connect_bd_net [get_bd_ports ap_clk] [get_bd_pins op_$id/ap_clk]
    connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins op_$id/ap_rst_n]

    set interrupt_id [expr $i - 1]
    connect_bd_net [get_bd_pins op_$id/interrupt] [get_bd_pins interrupt_concat/In${interrupt_id}]

    incr i
}

# AXI Perfmon

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_perf_mon:5.0 axi_perf_mon_0

set_property -dict [list \
    CONFIG.C_HAVE_SAMPLED_METRIC_CNT {0} \
    CONFIG.C_GLOBAL_COUNT_WIDTH {64} \
    CONFIG.C_NUM_OF_COUNTERS {10} \
    CONFIG.C_NUM_MONITOR_SLOTS $streams_count] [get_bd_cells axi_perf_mon_0]

for {set stream 0} {$stream < $streams_count} {incr stream} {
    set_property -dict [list CONFIG.C_SLOT_${stream}_AXI_PROTOCOL {AXI4S}] [get_bd_cells axi_perf_mon_0]
    connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_${stream}_AXIS] [get_bd_intf_pins metal_switch/M0${stream}_AXIS]
    connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_${stream}_axis_aclk]
    connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_${stream}_axis_aresetn]
}

connect_bd_intf_net [get_bd_intf_pins axi_metal_ctrl_crossbar/M00_AXI] [get_bd_intf_pins axi_perf_mon_0/S_AXI]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/s_axi_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/s_axi_aresetn]

connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/core_aclk]
connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/core_aresetn]
