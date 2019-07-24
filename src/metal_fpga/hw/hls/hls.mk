HLS_CFLAGS += $(ADDITIONAL_HLS_CFLAGS)

ifeq ($(DDRI_USED),TRUE)
	HLS_CFLAGS += -DDRAM_ENABLED
endif

ifeq ($(NVME_USED),TRUE)
	HLS_CFLAGS += -DNVME_ENABLED
endif

# run_hls_script.tcl: .hls_cflags

run_hls_ip_script.tcl: run_hls_script.tcl
	@sed "s/#export_design/export_design -library user -vendor user.org -version 1.0/g" $< > $@

run_hls_csim_script.tcl: run_hls_script.tcl
	@sed "s/csynth_design/csim_design -compiler clang/g" $< > $@

ip_dir=$(SOLUTION_DIR)_$(PART_NUMBER)/$(SOLUTION_NAME)/impl/ip

$(ip_dir): run_hls_ip_script.tcl $(srcs)
	vivado_hls -f run_hls_ip_script.tcl

# Create symlinks for simpler access
ip: $(ip_dir)
	@$(RM) hls_impl_ip && ln -s $(ip_dir) hls_impl_ip

test: run_hls_csim_script.tcl $(srcs)
	vivado_hls -f run_hls_csim_script.tcl

clean: clean_run_hls_ip_script clean_run_hls_csim_script clean_hls_cflags
clean_run_hls_ip_script:
	@$(RM) run_hls_ip_script.tcl
clean_run_hls_csim_script:
	@$(RM) run_hls_csim_script.tcl
clean_hls_cflags:
	@$(RM) .hls_cflags

$(srcs): .hls_cflags

# Special sauce: we want to re-make certain targets whenever $HLS_CFLAGS changes
# include $(METAL_ROOT)/third_party/make-utils/signature

.hls_cflags:
	@echo $(HLS_CFLAGS) > .hls_cflags
	# $(call do,echo \"$$(HLS_CFLAGS)\" > .hls_cflags)

# -include .sig
