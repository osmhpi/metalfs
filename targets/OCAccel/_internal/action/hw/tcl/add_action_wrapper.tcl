set build_dir       $::env(IMAGE_BUILD_DIR)/$::env(METAL_TARGET)
set include_dir     $::env(_OCACCEL_ROOT)/hardware/hdl/core/
add_files -norecurse $include_dir/snap_global_vars.v >> $log_file
add_files -norecurse $build_dir/action/action_wrapper.v >> $log_file
