set build_dir       $::env(IMAGE_BUILD_DIR)/$::env(METAL_TARGET)
add_files -norecurse $build_dir/action/action_wrapper.vhd >> $log_file
