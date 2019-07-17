## Env Variables

set action_root [lindex $argv 0]
set fpga_part  	[lindex $argv 1]

set aip_dir 	$action_root/ip/nvme_arbiter
set log_dir     $action_root
set log_file    $log_dir/create_nvme_arbiter_ip.log

## Create a new Vivado IP Project
puts "\[CREATE_ACTION_IPs..........\] start [clock format [clock seconds] -format {%T %a %b %d/ %Y}]"
puts "                        FPGACHIP = $fpga_part"
puts "                        ACTION_ROOT = $action_root"
create_project -force managed_ip_project $aip_dir/managed_ip_project -part $fpga_part

set_property target_language VHDL [current_project]
set_property simulator_language VHDL [current_project]
set_property target_simulator XSim [current_project]

add_files -norecurse \
    $aip_dir/fosi_ctrl.vhd \
    $aip_dir/fosi_nvme.vhd \
    $aip_dir/fosi_util.vhd \
    $aip_dir/NvmeController.vhd \
    $aip_dir/NvmeControllerWrapper.vhd \

update_compile_order -fileset sources_1

ipx::package_project -root_dir $aip_dir/ip_user_files -vendor user.org -library user -taxonomy /UserIP -import_files

ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]

close_project
puts "\[CREATE_ACTION_IPs..........\] done  [clock format [clock seconds] -format {%T %a %b %d %Y}]"