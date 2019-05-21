set_property ip_repo_paths [concat [get_property ip_repo_paths [current_project]] [glob -dir $action_hw_dir/hls */hls_impl_ip]] [current_project] >> $log_file
set_property ip_repo_paths [concat [get_property ip_repo_paths [current_project]] $action_dir/ip/ip_user_files] [current_project] >> $log_file
update_ip_catalog >> $log_file

create_ip -name bd_action -vendor user.org -library user -version 1.0 -module_name bd_action_0 >> $log_file
set_property synth_checkpoint_mode None [get_files $root_dir/viv_project/framework.srcs/sources_1/ip/bd_action_0/bd_action_0.xci] >> $log_file
generate_target all [get_files  $root_dir/viv_project/framework.srcs/sources_1/ip/bd_action_0/bd_action_0.xci] >> $log_file
config_ip_cache -export [get_ips -all bd_action_0] >> $log_file
