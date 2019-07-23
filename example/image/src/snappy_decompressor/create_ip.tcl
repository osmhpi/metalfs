## Env Variables

set operator_root [lindex $argv 0]
set fpga_part     [lindex $argv 1]

set log_file    $operator_root/create_ip.log
set src_dir     $operator_root/managed_ip_project/managed_ip_project.srcs/sources_1/ip/

## Create a new Vivado IP Project
create_project -force managed_ip_project $operator_root/managed_ip_project -part $fpga_part

set_property target_language VHDL [current_project]
set_property simulator_language VHDL [current_project]
set_property target_simulator XSim [current_project]

add_files -norecurse \
        $operator_root/SnappyWrapper.vhd \
        $operator_root/fifo.v \
        $operator_root/preparser.v \
        $operator_root/control.v \
        $operator_root/fifo_parser_copy.v \
        $operator_root/queue_token.v \
        $operator_root/copyread_selector.v   \
        $operator_root/fifo_parser_lit.v   \
        $operator_root/ram_block.v \
        $operator_root/copytoken_selector.v  \
        $operator_root/io_control.v \
        $operator_root/ram_module.v \
        $operator_root/data_out.v \
        $operator_root/lit_selector.v \
        $operator_root/select.v \
        $operator_root/decompressor.v \
        $operator_root/parser.v \
        $operator_root/distributor.v \
        $operator_root/parser_sub.v

create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.* -module_name blockram >> $log_file
set_property -dict [list                                              \
        CONFIG.Memory_Type {Simple_Dual_Port_RAM}                     \
        CONFIG.Assume_Synchronous_Clk {true}                          \
        CONFIG.Write_Width_A {72}                                     \
        CONFIG.Write_Depth_A {512}                                    \
        CONFIG.Read_Width_A {72}                                      \
        CONFIG.Operating_Mode_A {READ_FIRST}                          \
        CONFIG.Write_Width_B {72}                                     \
        CONFIG.Read_Width_B {72}                                      \
        CONFIG.Operating_Mode_B {READ_FIRST}                          \
        CONFIG.Enable_B {Use_ENB_Pin}                                 \
        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}     \
        CONFIG.Register_PortB_Output_of_Memory_Primitives {false}     \
        CONFIG.Port_B_Clock {100}                                     \
        CONFIG.Port_B_Enable_Rate {100}                               \
        CONFIG.Use_Byte_Write_Enable {true}                           \
        CONFIG.Fill_Remaining_Memory_Locations {true}                 \
        ] [get_ips blockram]

set_property generate_synth_checkpoint false [get_files $src_dir/blockram/blockram.xci] >> $log_file
generate_target {instantiation_template}     [get_files $src_dir/blockram/blockram.xci] >> $log_file
generate_target all                          [get_files $src_dir/blockram/blockram.xci] >> $log_file
export_ip_user_files -of_objects             [get_files $src_dir/blockram/blockram.xci] -no_script -force >> $log_file
export_simulation -of_objects                [get_files $src_dir/blockram/blockram.xci] -directory $operator_root/hls_impl_ip/sim_scripts -force >> $log_file


create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.* -module_name data_fifo >> $log_file
set_property -dict [list                                              \
        CONFIG.Fifo_Implementation {Common_Clock_Block_RAM}           \
        CONFIG.asymmetric_port_width {true}                           \
        CONFIG.Input_Data_Width {512}                                 \
        CONFIG.Input_Depth {512}                                      \
        CONFIG.Output_Data_Width {128}                                \
        CONFIG.Output_Depth {2048}                                    \
        CONFIG.Use_Embedded_Registers {false}                         \
        CONFIG.Almost_Full_Flag {true}                                \
        CONFIG.Valid_Flag {true}                                      \
        CONFIG.Use_Extra_Logic {true}                                 \
        CONFIG.Data_Count_Width {9}                                   \
        CONFIG.Write_Data_Count_Width {10}                            \
        CONFIG.Read_Data_Count_Width {12}                             \
        CONFIG.Programmable_Full_Type {Single_Programmable_Full_Threshold_Constant} \
        CONFIG.Full_Threshold_Assert_Value {500}                      \
        CONFIG.Full_Threshold_Negate_Value {499}                      \
        ] [get_ips data_fifo]

set_property generate_synth_checkpoint false [get_files $src_dir/data_fifo/data_fifo.xci] >> $log_file
generate_target {instantiation_template}     [get_files $src_dir/data_fifo/data_fifo.xci] >> $log_file
generate_target all                          [get_files $src_dir/data_fifo/data_fifo.xci] >> $log_file
export_ip_user_files -of_objects             [get_files $src_dir/data_fifo/data_fifo.xci] -no_script -force >> $log_file
export_simulation -of_objects                [get_files $src_dir/data_fifo/data_fifo.xci] -directory $operator_root/hls_impl_ip/sim_scripts -force >> $log_file


create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.* -module_name debugram
set_property -dict [list                                              \
        CONFIG.Memory_Type {Simple_Dual_Port_RAM}                     \
        CONFIG.Assume_Synchronous_Clk {true}                          \
        CONFIG.Write_Width_A {64}                                     \
        CONFIG.Write_Depth_A {512}                                    \
        CONFIG.Read_Width_A {64}                                      \
        CONFIG.Operating_Mode_A {READ_FIRST}                          \
        CONFIG.Write_Width_B {64}                                     \
        CONFIG.Read_Width_B {64}                                      \
        CONFIG.Operating_Mode_B {READ_FIRST}                          \
        CONFIG.Enable_B {Use_ENB_Pin}                                 \
        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}     \
        CONFIG.Register_PortB_Output_of_Memory_Primitives {false}     \
        CONFIG.Port_B_Clock {100}                                     \
        CONFIG.Port_B_Enable_Rate {100}                               \
        CONFIG.Use_Byte_Write_Enable {true}                           \
        CONFIG.Byte_Size {8}                                          \
        CONFIG.Write_Width_A {64}                                     \
        CONFIG.Read_Width_A {64}                                      \
        CONFIG.Fill_Remaining_Memory_Locations {true}                 \
        ] [get_ips debugram]

set_property generate_synth_checkpoint false [get_files $src_dir/debugram/debugram.xci] >> $log_file
generate_target {instantiation_template}     [get_files $src_dir/debugram/debugram.xci] >> $log_file
generate_target all                          [get_files $src_dir/debugram/debugram.xci] >> $log_file
export_ip_user_files -of_objects             [get_files $src_dir/debugram/debugram.xci] -no_script -force >> $log_file
export_simulation -of_objects                [get_files $src_dir/debugram/debugram.xci] -directory $operator_root/hls_impl_ip/sim_scripts -force >> $log_file


create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.* -module_name page_fifo
set_property -dict [list                                              \
        CONFIG.Input_Data_Width {181}                                 \
        CONFIG.Input_Depth {512}                                      \
        CONFIG.Output_Data_Width {181}                                \
        CONFIG.Output_Depth {512}                                     \
        CONFIG.Valid_Flag {true}                                      \
        CONFIG.Data_Count_Width {9}                                   \
        CONFIG.Write_Data_Count_Width {9}                             \
        CONFIG.Read_Data_Count_Width {9}                              \
        CONFIG.Programmable_Full_Type {Single_Programmable_Full_Threshold_Constant} \
        CONFIG.Full_Threshold_Assert_Value {500}                      \
        CONFIG.Full_Threshold_Negate_Value {499}                      \
        ] [get_ips page_fifo]

set_property generate_synth_checkpoint false [get_files $src_dir/page_fifo/page_fifo.xci] >> $log_file
generate_target {instantiation_template}     [get_files $src_dir/page_fifo/page_fifo.xci] >> $log_file
generate_target all                          [get_files $src_dir/page_fifo/page_fifo.xci] >> $log_file
export_ip_user_files -of_objects             [get_files $src_dir/page_fifo/page_fifo.xci] -no_script -force >> $log_file
export_simulation -of_objects                [get_files $src_dir/page_fifo/page_fifo.xci] -directory $operator_root/hls_impl_ip/sim_scripts -force >> $log_file


create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.* -module_name result_ram
set_property -dict [list                                             \
        CONFIG.Memory_Type {Simple_Dual_Port_RAM}                     \
        CONFIG.Use_Byte_Write_Enable {true}                           \
        CONFIG.Write_Width_A {72}                                     \
        CONFIG.Write_Depth_A {512}                                    \
        CONFIG.Read_Width_A {72}                                      \
        CONFIG.Operating_Mode_A {READ_FIRST}                          \
        CONFIG.Write_Width_B {72}                                     \
        CONFIG.Read_Width_B {72}                                      \
        CONFIG.Enable_B {Use_ENB_Pin}                                 \
        CONFIG.Register_PortA_Output_of_Memory_Primitives {false}     \
        CONFIG.Register_PortB_Output_of_Memory_Primitives {false}     \
        CONFIG.Fill_Remaining_Memory_Locations {true}                 \
        CONFIG.Port_B_Clock {100}                                     \
        CONFIG.Port_B_Enable_Rate {100}                               \
        ] [get_ips result_ram]

set_property generate_synth_checkpoint false [get_files $src_dir/result_ram/result_ram.xci] >> $log_file
generate_target {instantiation_template}     [get_files $src_dir/result_ram/result_ram.xci] >> $log_file
generate_target all                          [get_files $src_dir/result_ram/result_ram.xci] >> $log_file
export_ip_user_files -of_objects             [get_files $src_dir/result_ram/result_ram.xci] -no_script -force >> $log_file
export_simulation -of_objects                [get_files $src_dir/result_ram/result_ram.xci] -directory $operator_root/hls_impl_ip/sim_scripts -force >> $log_file


create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.* -module_name unsolved_fifo
set_property -dict [list                                              \
        CONFIG.Fifo_Implementation {Common_Clock_Block_RAM}           \
        CONFIG.Input_Data_Width {33}                                  \
        CONFIG.Input_Depth {512}                                      \
        CONFIG.Output_Data_Width {33}                                 \
        CONFIG.Output_Depth {512}                                     \
        CONFIG.Use_Embedded_Registers {false}                         \
        CONFIG.Almost_Full_Flag {true}                                \
        CONFIG.Valid_Flag {true}                                      \
        CONFIG.Data_Count {true}                                      \
        CONFIG.Data_Count_Width {9}                                   \
        CONFIG.Write_Data_Count_Width {9}                             \
        CONFIG.Read_Data_Count_Width {9}                              \
        CONFIG.Programmable_Full_Type {Single_Programmable_Full_Threshold_Constant} \
        CONFIG.Full_Threshold_Assert_Value {450}                      \
        CONFIG.Full_Threshold_Negate_Value {449}                      \
        ] [get_ips unsolved_fifo]

set_property generate_synth_checkpoint false [get_files $src_dir/unsolved_fifo/unsolved_fifo.xci] >> $log_file
generate_target {instantiation_template}     [get_files $src_dir/unsolved_fifo/unsolved_fifo.xci] >> $log_file
generate_target all                          [get_files $src_dir/unsolved_fifo/unsolved_fifo.xci] >> $log_file
export_ip_user_files -of_objects             [get_files $src_dir/unsolved_fifo/unsolved_fifo.xci] -no_script -force >> $log_file
export_simulation -of_objects                [get_files $src_dir/unsolved_fifo/unsolved_fifo.xci] -directory $operator_root/hls_impl_ip/sim_scripts -force >> $log_file


set main [exec jq -r .main $operator_root/operator.json]
set_property top $main [current_fileset]
update_compile_order -fileset sources_1

ipx::package_project -root_dir $operator_root/hls_impl_ip -vendor user.org -library user -taxonomy /UserIP -import_files

ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]

close_project

