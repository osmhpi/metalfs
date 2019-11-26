## Env Variables

set aip_dir     [lindex $argv 0]
set fpga_part   [lindex $argv 1]
set log_dir     [lindex $argv 2]

set log_file    $log_dir/create_action_ip.log
set src_dir     $aip_dir/action_ip_prj/action_ip_prj.srcs/sources_1/ip

set image_json  $::env(IMAGE_JSON)

## Create a new Vivado IP Project
create_project action_ip_prj $aip_dir/action_ip_prj -force -part $fpga_part -ip >> $log_file
set_property target_language VHDL [current_project]

set_property ip_repo_paths [concat \
    [glob -dir $aip_dir/../../hw/hls */hls_impl_ip] \
    $aip_dir/../image_info/ip_user_files \
    $aip_dir/../nvme_arbiter/ip_user_files \
    [eval list [exec sh -c "$aip_dir/../../hw/resolve_operators $image_json | cut -f2 | uniq | sed -e 's=$=/hls_impl_ip=' | paste -s -d ' '"]] \
] [current_project] >> $log_file
update_ip_catalog >> $log_file

source $aip_dir/create_bd.tcl

close_project
