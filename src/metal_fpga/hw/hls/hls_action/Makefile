SOLUTION_NAME ?= metal_hls_action
SOLUTION_DIR ?= metal_hls_action
srcs += axi_switch.cpp \
	mtl_op_mem.cpp \
	mtl_op_passthrough.cpp \
	mtl_op_image.cpp \
	mtl_op_change_case.cpp \
	hls_metalfpga.cpp

HLS_CFLAGS ?= ""
ifeq ($(DDRI_USED),TRUE)
HLS_CFLAGS += -DDRAM_ENABLED
endif
ifeq ($(NVME_USED),TRUE)
HLS_CFLAGS += -DNVME_ENABLED
endif

include $(SNAP_ROOT)/actions/hls.mk

run_hls_ip_script.tcl: run_hls_script.tcl
	@sed "s/#export_design/export_design/g" $< > $@

ip_dir=$(SOLUTION_DIR)_$(PART_NUMBER)/$(SOLUTION_NAME)/impl/ip

$(ip_dir): run_hls_ip_script.tcl $(srcs)
	vivado_hls -f run_hls_ip_script.tcl

# Create symlinks for simpler access
ip: $(ip_dir)
	@$(RM) hls_impl_ip && ln -s $(ip_dir) hls_impl_ip

clean: clean_run_hls_ip_script
clean_run_hls_ip_script:
	@$(RM) run_hls_ip_script.tcl
