set build_dir       $::env(IMAGE_BUILD_DIR)/$::env(SNAP_TARGET)
add_files -scan_for_includes $build_dir/action/ >> $log_file
