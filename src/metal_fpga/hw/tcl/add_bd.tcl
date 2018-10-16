set_property ip_repo_paths [glob -dir $action_hw_dir/hls */hls_impl_ip] [current_project] >> $log_file
update_ip_catalog >> $log_file

add_files -norecurse                          $action_ip_dir/../bd/action_wrapper/action_wrapper.bd  >> $log_file
export_ip_user_files -of_objects  [get_files  $action_ip_dir/../bd/action_wrapper/action_wrapper.bd] -lib_map_path [list {{ies=$root_dir/viv_project/framework.cache/compile_simlib/ies}}] -no_script -sync -force -quiet
