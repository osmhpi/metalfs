BUILD_DIR     ?= build
WRAPPER       ?= $(shell jq -r .main operator.json)

ip: $(BUILD_DIR)/ip_user_files/component.xml

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/create_ip.tcl: $(BUILD_DIR)
	$(METAL_ROOT)/buildpacks/hdl/scripts/create_ip_script.sh	\
		-w $(WRAPPER)	\
		-p $(FPGACHIP)	\
		-f "$(srcs)"	> $@

$(BUILD_DIR)/ip_user_files/component.xml: $(srcs) $(BUILD_DIR)/create_ip.tcl
	@cd $(BUILD_DIR) && vivado -quiet -mode batch -source create_ip.tcl -nolog -notrace -nojournal

clean:
	@rm -rf $(BUILD_DIR)
