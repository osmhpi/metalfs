## Env Variables

set aip_dir     [lindex $argv 0]
set build_dir   [lindex $argv 1]
set fpga_part   [lindex $argv 2]
set log_dir     [lindex $argv 3]

set log_file    $log_dir/create_block_design.log
set src_dir     $build_dir/block_design/action_ip_prj/action_ip_prj.srcs/sources_1/ip

set image_build_dir  $::env(IMAGE_BUILD_DIR)/$::env(METAL_TARGET)
set image_json       $::env(IMAGE_JSON)
set metal_root       $::env(METAL_ROOT)

## Create a new Vivado IP Project
create_project action_ip_prj $build_dir/block_design/action_ip_prj -force -part $fpga_part -ip >> $log_file
set_property target_language VHDL [current_project]

set_property ip_repo_paths [concat \
    [glob -dir $build_dir */hls_impl_ip] \
    [glob -dir $build_dir */ip_user_files] \
    [eval list [exec sh -c "$metal_root/buildpacks/image/scripts/resolve_operators $image_json | cut -f2 | xargs basename | uniq | sed -e 's=^=$image_build_dir/operators/=' | paste -s -d ' '"]] \
] [current_project] >> $log_file
update_ip_catalog >> $log_file

source $aip_dir/create_bd.tcl

close_project
