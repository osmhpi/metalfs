## Env Variables

set ip_dir      [lindex $argv 0]
set fpga_part   [lindex $argv 1]
set log_dir     [lindex $argv 2]

set log_file    $log_dir/create_nvme_arbiter_ip.log

## Create a new Vivado IP Project
create_project -force managed_ip_project $ip_dir/managed_ip_project -part $fpga_part

set_property target_language VHDL [current_project]
set_property simulator_language VHDL [current_project]
set_property target_simulator XSim [current_project]

add_files -norecurse \
    $ip_dir/nvme_def.vhd \
    $ip_dir/NvmeController.vhd

update_compile_order -fileset sources_1

ipx::package_project -root_dir $ip_dir/ip_user_files -vendor user.org -library user -taxonomy /UserIP -import_files >> $log_file

ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]

close_project
