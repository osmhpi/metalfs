set image_json      $::env(IMAGE_JSON)
set image_target    $::env(IMAGE_TARGET)
set operators       [eval dict create [split [exec sh -c "$metal_root/buildpacks/image/scripts/resolve_operators $image_json | paste -s -d ' '" ]]]

set streams_count [expr [dict size $operators] + 1]

set_property -dict [list CONFIG.NUM_SI $streams_count CONFIG.NUM_MI $streams_count] [get_bd_cells metal_switch]
set_property -dict [list CONFIG.NUM_MI [expr [dict size $operators] + 5]] [get_bd_cells axi_metal_ctrl_crossbar]

set i 0

dict for {id path} $operators {
    set internal_id [ exec jq --arg id $id -r ".operators\[\$id\].internal_id" $image_target ]

    set component [exec jq -r .main $path/operator.json]
    create_bd_cell -type ip -vlnv user.org:user:$component:1.0 op_$id

    connect_bd_intf_net [get_bd_intf_pins op_$id/axis_output] [get_bd_intf_pins metal_switch/S0${internal_id}_AXIS]
    connect_bd_intf_net [get_bd_intf_pins metal_switch/M0${internal_id}_AXIS] [get_bd_intf_pins op_${id}/axis_input]
    # Disable connectivity to itself
    set_property -dict [list CONFIG.M0${internal_id}_S0${internal_id}_CONNECTIVITY {0}] [get_bd_cells metal_switch]

    set control_port [expr $internal_id + 4]
    connect_bd_intf_net [get_bd_intf_pins axi_metal_ctrl_crossbar/M0${control_port}_AXI] [get_bd_intf_pins op_$id/s_axi_control]

    connect_bd_net [get_bd_ports ap_clk] [get_bd_pins op_$id/ap_clk]
    connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins op_$id/ap_rst_n]

    set interrupt_id [expr $internal_id - 1]
    connect_bd_net [get_bd_pins op_$id/interrupt] [get_bd_pins interrupt_concat/In${interrupt_id}]

    delete_bd_objs [get_bd_addr_segs -excluded snap_action/Data_m_axi_metal_ctrl_V/SEG_op_${id}_Reg*]  >> $log_file
    assign_bd_address [get_bd_addr_segs {op_${id}/s_axi_control/Reg }]  >> $log_file
    include_bd_addr_seg [get_bd_addr_segs -excluded snap_action/Data_m_axi_metal_ctrl_V/SEG_op_${id}_Reg*]  >> $log_file

    set i [expr max($i, $internal_id)]
}

incr i

# Initialize remaining interrupt ports with constant 0

if { $i <= 8 } {
    create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 interrupt_zero
    set_property -dict [list CONFIG.CONST_VAL {0}] [get_bd_cells interrupt_zero]
}

while {$i <= 8} {
    set interrupt_id [expr $i - 1]
    connect_bd_net [get_bd_pins interrupt_zero/dout] [get_bd_pins interrupt_concat/In${interrupt_id}]
    incr i
}

# Connect to perf mon

set_property -dict [list \
    CONFIG.C_NUM_MONITOR_SLOTS $streams_count] [get_bd_cells axi_perf_mon_0]

for {set stream 0} {$stream < $streams_count} {incr stream} {
    set_property -dict [list CONFIG.C_SLOT_${stream}_AXI_PROTOCOL {AXI4S}] [get_bd_cells axi_perf_mon_0]
    connect_bd_intf_net [get_bd_intf_pins axi_perf_mon_0/SLOT_${stream}_AXIS] [get_bd_intf_pins metal_switch/M0${stream}_AXIS]
    connect_bd_net [get_bd_ports ap_clk] [get_bd_pins axi_perf_mon_0/slot_${stream}_axis_aclk]
    connect_bd_net [get_bd_ports ap_rst_n] [get_bd_pins axi_perf_mon_0/slot_${stream}_axis_aresetn]
}
