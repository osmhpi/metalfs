BUILD_DIR     ?= build

SOLUTION_NAME ?= hls_component
SOLUTION_DIR  ?= hls_solution

# Assume some defaults
FPGACHIP         ?= xcku035-ffva1156-2-e
HLS_ACTION_CLOCK ?= 4
STREAM_BYTES     ?= $(shell jq -r '.devImage.streamBytes // 8' operator.json)

HLS_CFLAGS += \
	-std=c++11 \
	-I$(METAL_ROOT)/buildpacks/hls/include \
	-DSTREAM_BYTES=$(STREAM_BYTES)

WRAPPER ?= $(shell jq -r .main operator.json)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/run_hls_script.tcl: $(BUILD_DIR)/hls_cflags.cache
	@$(METAL_ROOT)/buildpacks/hls/scripts/create_run_hls_script.sh	\
		-n $(SOLUTION_NAME)		\
		-d $(SOLUTION_DIR)		\
		-w $(WRAPPER)			\
		-p $(FPGACHIP)		\
		-c $(HLS_ACTION_CLOCK)	\
		-f "$(srcs)" 			\
		-t "$(testbench_srcs)"	\
		-x "$(HLS_CFLAGS)" > $@

$(BUILD_DIR)/run_hls_ip_script.tcl: $(BUILD_DIR)/run_hls_script.tcl
	@sed "s/#export_design/export_design -library user -vendor user.org -version 1.0/g" $< > $@

$(BUILD_DIR)/run_hls_csim_script.tcl: $(BUILD_DIR)/run_hls_script.tcl
	@sed "s/csynth_design/csim_design -compiler clang/g" $< > $@

ip_dir=$(SOLUTION_DIR)_$(FPGACHIP)/$(SOLUTION_NAME)/impl/ip

$(BUILD_DIR)/$(ip_dir): $(BUILD_DIR)/run_hls_ip_script.tcl $(srcs)
	@cd $(BUILD_DIR) && vivado_hls -f run_hls_ip_script.tcl

ip: $(BUILD_DIR)/$(ip_dir)
	@cd $(BUILD_DIR) && rm -f hls_impl_ip && ln -s $(ip_dir) hls_impl_ip

test: $(BUILD_DIR)/run_hls_csim_script.tcl $(srcs)
	@cd $(BUILD_DIR) && vivado_hls -f run_hls_csim_script.tcl

clean: 
	@rm -rf $(BUILD_DIR)

# Special sauce: we want to re-make certain targets whenever $HLS_CFLAGS changes.
include $(METAL_ROOT)/third_party/make-utils/gmsl
include $(METAL_ROOT)/third_party/make-utils/signature

%.cache:
	@$(call do,echo \"$$(HLS_CFLAGS)\" > $$@)

$(BUILD_DIR)/hls_cflags.sig: $(BUILD_DIR)
-include $(BUILD_DIR)/hls_cflags.sig
