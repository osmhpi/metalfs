set image_json      $::env(IMAGE_JSON)
set metal_root      $::env(METAL_ROOT)
set snap_target     $::env(SNAP_TARGET)

set_property ip_repo_paths [concat \
    [get_property ip_repo_paths [current_project]] \
    [glob -dir $metal_root/targets/SNAP/build/$snap_target/ip */hls_impl_ip] \
    $action_dir/ip/image_info/build/ip_user_files \
    $action_dir/ip/nvme_arbiter/build/ip_user_files \
    [eval list [exec sh -c "$metal_root/buildpacks/image/scripts/resolve_operators $image_json | cut -f2 | uniq | sed -e 's=$=/build/hls_impl_ip=' | paste -s -d ' '"]] \
    $action_dir/ip/block_design/ip_user_files \
] [current_project] >> $log_file
update_ip_catalog >> $log_file

# create_ip -name bd_action -vendor user.org -library user -version 1.0 -module_name bd_action_0 >> $log_file
# set_property synth_checkpoint_mode None [get_files $root_dir/viv_project/framework.srcs/sources_1/ip/bd_action_0/bd_action_0.xci] >> $log_file
# generate_target all [get_files  $root_dir/viv_project/framework.srcs/sources_1/ip/bd_action_0/bd_action_0.xci] >> $log_file
# config_ip_cache -export [get_ips -all bd_action_0] >> $log_file

set action_bd_dir  $action_dir/ip/block_design/action_ip_prj/action_ip_prj.srcs/sources_1/bd
add_files -norecurse                          $action_bd_dir/bd_action/bd_action.bd  >> $log_file
export_ip_user_files -of_objects  [get_files  $action_bd_dir/bd_action/bd_action.bd] -lib_map_path [list {{ies=$root_dir/viv_project/framework.cache/compile_simlib/ies}}] -no_script -sync -force -quiet
