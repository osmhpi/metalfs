BUILD_DIR     ?= build
WRAPPER       ?= $(shell jq -r .main operator.json)

STREAM_BYTES     ?= $(shell jq -r '.devImage.streamBytes // 8' operator.json)
DEVIMAGE_TARGET  ?= SNAP/WebPACK_Sim

ip: $(BUILD_DIR)/ip_user_files/component.xml

$(BUILD_DIR)/create_ip.tcl:
	mkdir -p $(BUILD_DIR)
	$(METAL_ROOT)/buildpacks/hdl/scripts/create_ip_script.sh	\
		-w $(WRAPPER)	\
		-p $(FPGACHIP)	\
		-f "$(srcs)"	> $@

$(BUILD_DIR)/ip_user_files/component.xml: $(srcs) $(BUILD_DIR)/create_ip.tcl
	@cd $(BUILD_DIR) && vivado -quiet -mode batch -source create_ip.tcl -nolog -notrace -nojournal

clean:
	@rm -rf $(BUILD_DIR)

help:
	@echo "Targets in the HDL Operator Buildpack";
	@echo "=====================================";
	@echo "* ip             Build Vivado IP";
	@echo "* devmodel       Build a simulation model containing only the current operator";
	@echo "* sim            Start a simulation with the devmodel";
	@echo "* clean          Remove the build directory";
	@echo "* help           Print this message";
	@echo;

# Development image targets

build/devimage.json:
	@mkdir -p build
	@echo '{ "streamBytes": $(STREAM_BYTES), "target": "$(DEVIMAGE_TARGET)", "operators": { "$(WRAPPER)": ".." } }' > build/devimage.json

devmodel: build/devimage.json
	@make -f $(METAL_ROOT)/buildpacks/image/image.mk IMAGE_JSON=$(PWD)/build/devimage.json model

sim: build/devimage.json
	@make -f $(METAL_ROOT)/buildpacks/image/image.mk IMAGE_JSON=$(PWD)/build/devimage.json sim
