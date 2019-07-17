## Env Variables

set action_root [lindex $argv 0]
set fpga_part   [lindex $argv 1]

set aip_dir     $action_root/ip/block_design
set log_dir     $action_root
set log_file    $log_dir/create_action_ip.log
set src_dir     $aip_dir/action_ip_prj/action_ip_prj.srcs/sources_1/ip

set image_json  $::env(IMAGE_JSON)

## Create a new Vivado IP Project
puts "\[CREATE_ACTION_IPs..........\] start [clock format [clock seconds] -format {%T %a %b %d/ %Y}]"
puts "                        FPGACHIP = $fpga_part"
puts "                        ACTION_ROOT = $action_root"
puts "                        Creating IP in $src_dir"
create_project action_ip_prj $aip_dir/action_ip_prj -force -part $fpga_part -ip >> $log_file
set_property target_language VHDL [current_project]

puts "                        Configuring IP Catalog ......"
set_property ip_repo_paths [concat \
    [glob -dir $action_root/hw/hls */hls_impl_ip] \
    $action_root/ip/image_info/ip_user_files \
    $action_root/ip/nvme_arbiter/ip_user_files \
    [eval list [exec sh -c "$action_root/hw/resolve_operators $image_json | cut -f2 | uniq | sed -e 's=$=/hls_impl_ip=' | paste -s -d ' '"]] \
] [current_project] >> $log_file
update_ip_catalog >> $log_file

puts "                        Generating Block Design ......"
source $aip_dir/create_bd.tcl

close_project
puts "\[CREATE_ACTION_IPs..........\] done  [clock format [clock seconds] -format {%T %a %b %d %Y}]"
