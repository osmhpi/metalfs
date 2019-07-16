## Env Variables

set action_root [lindex $argv 0]
set fpga_part  	[lindex $argv 1]

set aip_dir 	$action_root
set log_dir     $action_root
set log_file    $log_dir/create_action_ip.log
set src_dir 	$aip_dir/action_ip_prj/action_ip_prj.srcs/sources_1/ip

## Create a new Vivado IP Project
puts "\[CREATE_ACTION_IPs..........\] start [clock format [clock seconds] -format {%T %a %b %d/ %Y}]"
puts "                        FPGACHIP = $fpga_part"
puts "                        ACTION_ROOT = $action_root"
puts "                        Creating IP in $src_dir"
# create_project action_ip_prj $aip_dir/action_ip_prj -force -part $fpga_part -ip >> $log_file

# add_files -norecurse $action_root/config_store.vhd

create_project -force managed_ip_project $aip_dir/managed_ip_project -part $fpga_part
set_property target_language VHDL [current_project]
set_property simulator_language VHDL [current_project]
set_property target_simulator XSim [current_project]

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
# 							create the TDM8 RX IP Core
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

#create_peripheral xilinx.com user TDM8_rx_AXIS 0.2 -dir $aip_dir/managed_ip_project/../ip_repo
#add_peripheral_interface M_AXIS -interface_mode master -axi_type stream [ipx::find_open_core xilinx.com:user:TDM8_rx_AXIS:0.2]
#generate_peripheral [ipx::find_open_core xilinx.com:user:TDM8_rx_AXIS:0.2]
#write_peripheral [ipx::find_open_core xilinx.com:user:TDM8_rx_AXIS:0.2]
set_property  ip_repo_paths  $aip_dir/managed_ip_project/../ip_repo/TDM8_rx_AXIS_0.2 [current_project]
update_ip_catalog -rebuild

ipx::edit_ip_in_project -upgrade true -name edit_TDM8_rx_AXIS_v0_2 -directory $aip_dir/managed_ip_project/../ip_repo $aip_dir/ip_repo/TDM8_rx_AXIS_0.2/component.xml

set_property vendor ThomasMaintz [ipx::current_core]
set_property library TMAThesis [ipx::current_core]
set_property description {Converts the TDM8 Slave Receiver into the AXI Stream as a Master} [ipx::current_core]
add_files -norecurse $action_root/config_store.vhd
#add_files -norecurse -copy_to $aip_dir/ip_repo/TDM8_rx_AXIS_0.2/src { VHDL/Code/PCM/pcm_package.vhd  VHDL/Code/TDM8/tdm8_package.vhd  VHDL/Code/TDM8/tdm8_trx.vhd} -force

#set_property value_validation_list {32 24} [ipx::get_user_parameters C_M_AXIS_TDATA_WIDTH -of_objects [ipx::current_core]]
#set_property widget {textEdit} [ipgui::get_guiparamspec -name "C_M_AXIS_START_COUNT" -component [ipx::current_core] ]
#set_property value_validation_range_minimum 1 [ipx::get_user_parameters C_M_AXIS_START_COUNT -of_objects [ipx::current_core]]
#set_property value_validation_range_maximum 64 [ipx::get_user_parameters C_M_AXIS_START_COUNT -of_objects [ipx::current_core]]

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
#               NOW COPY AND PASTE THE IP CORE VHDL FILES
#               FROM THE TDM8 DIRECTORY INTO THE CREATED
#               VHDL FILES!
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

update_compile_order -fileset sources_1
ipx::merge_project_changes files [ipx::current_core]

set_property previous_version_for_upgrade xilinx.com:user:TDM8_rx_AXIS:0.2 [ipx::current_core]
set_property core_revision 1 [ipx::current_core]
ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]
close_project -delete
update_ip_catalog -rebuild -repo_path $aip_dir/ip_repo/TDM8_rx_AXIS_0.2

close_project
puts "\[CREATE_ACTION_IPs..........\] done  [clock format [clock seconds] -format {%T %a %b %d %Y}]"