open_project "metal_hls_action_xcku060-ffva1156-2-e"

set_top hls_action

# Can that be a list?
foreach file [ list mtl_extmap.cpp mtl_nvme.cpp mtl_file.cpp mtl_jobstruct.cpp mtl_op_mem.cpp mtl_op_file.cpp mtl_op_passthrough.cpp mtl_op_change_case.cpp mtl_op_blowfish.cpp hls_metalfpga.cpp ] {
  add_files ${file} -cflags "-I/home/robert/code/fssem/metal_fs/snap/actions/include -I/home/robert/code/fssem/metal_fs/snap/software/include -I../../../software/examples -I../include"
  add_files -tb ${file} -cflags "-DNO_SYNTH -I/home/robert/code/fssem/metal_fs/snap/actions/include -I/home/robert/code/fssem/metal_fs/snap/software/include -I../../../software/examples -I../include"
}

open_solution "metal_hls_action"
set_part xcku060-ffva1156-2-e

create_clock -period 4 -name default
config_interface -m_axi_addr64=true
#config_rtl -reset all -reset_level low

csynth_design
export_design -format ip_catalog -rtl vhdl
exit
