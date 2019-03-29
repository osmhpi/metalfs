ifeq ($(HLS_CFLAGS),"")
	HLS_CFLAGS :=
endif

ifeq ($(DDRI_USED),TRUE)
	HLS_CFLAGS += -DDRAM_ENABLED
endif

ifeq ($(NVME_USED),TRUE)
	HLS_CFLAGS += -DNVME_ENABLED
endif

run_hls_script: .hls_cflags

run_hls_ip_script.tcl: run_hls_script.tcl
	@sed "s/#export_design/export_design/g" $< > $@

ip_dir=$(SOLUTION_DIR)_$(PART_NUMBER)/$(SOLUTION_NAME)/impl/ip

$(ip_dir): run_hls_ip_script.tcl $(srcs)
	vivado_hls -f run_hls_ip_script.tcl

# Create symlinks for simpler access
ip: $(ip_dir)
	@$(RM) hls_impl_ip && ln -s $(ip_dir) hls_impl_ip

clean: clean_run_hls_ip_script clean_hls_cflags
clean_run_hls_ip_script:
	@$(RM) run_hls_ip_script.tcl
clean_hls_cflags:
	@$(RM) .hls_cflags

# Make srcs dependent on the currently set HLS CFLAGS
$(srcs): .hls_cflags

.hls_cflags: FORCE
	@if [ -n .hls_cflags ]; then touch .hls_cflags; fi
	@if [ $$(echo "$(HLS_CFLAGS)" | comm -3 .hls_cflags - | wc -c) -gt 0 ]; then echo "$(HLS_CFLAGS)" > .hls_cflags; fi

FORCE:
