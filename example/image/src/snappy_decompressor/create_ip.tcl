## Env Variables

set operator_root [lindex $argv 0]
set fpga_part     [lindex $argv 1]

set log_file    $operator_root/create_ip.log

## Create a new Vivado IP Project
create_project -force managed_ip_project $operator_root/managed_ip_project -part $fpga_part

set_property target_language VHDL [current_project]
set_property simulator_language VHDL [current_project]
set_property target_simulator XSim [current_project]

# add_files -norecurse \
#     $operator_root/fosi_ctrl.vhd \
#     $operator_root/fosi_nvme.vhd \
#     $operator_root/fosi_util.vhd \
#     $operator_root/NvmeController.vhd \
#     $operator_root/NvmeControllerWrapper.vhd \

update_compile_order -fileset sources_1

ipx::package_project -root_dir $operator_root/hls_impl_ip -vendor user.org -library user -taxonomy /UserIP -import_files

ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]

close_project
