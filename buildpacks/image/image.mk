ifndef METAL_ROOT
$(info You have specified a wrong $$METAL_ROOT.)
$(error Please make sure that $$METAL_ROOT is set up correctly.)
endif

BUILD_DIR           = build
LOGS_DIR            = logs
export IMAGE_JSON   = $(PWD)/image.json
export IMAGE_TARGET = $(PWD)/$(BUILD_DIR)/image.json
export METAL_TARGET = $(shell jq -r .target $(IMAGE_JSON))

targets   = overlay model sim image
operators = $(shell $(METAL_ROOT)/buildpacks/image/scripts/resolve_operators $(IMAGE_JSON) | cut -f 2 | uniq)

all: help

# Load the target platform Makefile
# It must define the following targets:
#  - overlay         Assemble the Metal FS overlay from User IP
#  - model           Create simulation model
#  - sim             Start simulation
#  - image           Create FPGA image
#  - help            (optional)
#  - clean
include $(METAL_ROOT)/targets/$(METAL_TARGET)/../target.mk

# Additional dependencies:
$(targets): $(IMAGE_TARGET) $(operators)
clean: clean_operators

clean_operators:
	@for dir in $(operators); do make -C $$dir -s clean; done

$(IMAGE_TARGET): $(IMAGE_JSON) $(BUILD_DIR)
	echo $(operators)
	@$(METAL_ROOT)/buildpacks/image/scripts/generate_image $(IMAGE_JSON) $(IMAGE_TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Operators
$(operators):
	@echo "                        Generating IP from Metal FS Operator $(@F)"
	@make -C $@ ip; hls_ret=$$?; \
	if [ $${hls_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/$(@F)_make.log"; exit -1; \
	fi
