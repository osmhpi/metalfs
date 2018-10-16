SOLUTION_NAME ?= metal_hls_action
SOLUTION_DIR ?= metal_hls_action
srcs += mtl_extmap.cpp \
	mtl_nvme.cpp \
	mtl_file.cpp \
	mtl_jobstruct.cpp \
	mtl_op_mem.cpp \
	mtl_op_file.cpp \
	mtl_op_passthrough.cpp \
	mtl_op_change_case.cpp \
	mtl_op_blowfish.cpp \
	hls_metalfpga.cpp

include $(SNAP_ROOT)/actions/hls.mk

run_hls_ip_script.tcl: run_hls_script.tcl
	sed "s/#export_design/export_design/g" $< > $@

ip_dir=$(SOLUTION_DIR)_$(PART_NUMBER)/$(SOLUTION_NAME)/impl/ip

$(ip_dir): run_hls_ip_script.tcl
	vivado_hls -f run_hls_ip_script.tcl

# Create symlinks for simpler access
ip: $(ip_dir)
	@$(RM) hls_impl_ip && ln -s $(ip_dir) hls_impl_ip
