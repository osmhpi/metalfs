ifndef METAL_ROOT
$(info You have specified a wrong $$METAL_ROOT.)
$(error Please make sure that $$METAL_ROOT is set up correctly.)
endif

export IMAGE_BUILD_DIR  = $(PWD)/build
export IMAGE_JSON      ?= $(PWD)/image.json
export IMAGE_TARGET     = $(IMAGE_BUILD_DIR)/image.json
export METAL_TARGET     = $(shell jq -r .target $(IMAGE_JSON))

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
# As well as the following variables:
#  - LOGS_DIR
#  - OPERATORS_BUILD_DIR
include $(METAL_ROOT)/targets/$(METAL_TARGET)/../target.mk

# Additional dependencies:
$(targets): $(IMAGE_TARGET) $(operators)
clean: clean_operators

clean_operators:
	@for dir in $(operators); do make -C $$dir -s clean; done

$(IMAGE_TARGET): $(IMAGE_JSON) $(IMAGE_BUILD_DIR)
	@$(METAL_ROOT)/buildpacks/image/scripts/generate_image $(IMAGE_JSON) $(IMAGE_TARGET)

$(IMAGE_BUILD_DIR):
	@mkdir -p $(IMAGE_BUILD_DIR)

$(LOGS_DIR):
	@mkdir -p $(LOGS_DIR)

.FORCE:

# Operators
$(operators): .FORCE $(LOGS_DIR)
	@echo "                        Generating IP from Metal FS Operator $(@F)"
	@mkdir -p $(OPERATORS_BUILD_DIR)/$(@F)
	@make -C $@ BUILD_DIR="$(OPERATORS_BUILD_DIR)/$(@F)" ip >> $(LOGS_DIR)/operator_$(@F)_make.log; op_ret=$$?; \
	if [ $${op_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/operator_$(@F)_make.log"; exit -1; \
	fi
