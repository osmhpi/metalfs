export LOGS_DIR            = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/logs
export OPERATORS_BUILD_DIR = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/operators

# Define underscored variables as not to interfere with externally set values
export _HDK_ROOT    = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/aws-fpga
export _CL_DIR      = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/cl

export FPGACHIP=xcvu9p-flgb2104-2-i

overlay: hw_project

$(_HDK_ROOT):
	@mkdir -p $(LOGS_DIR)
	@if [ ! -e $(_HDK_ROOT) ] ; then \
		echo "                        Cloning AWS HDK..." ; \
		git clone https://github.com/aws/aws-fpga $(_HDK_ROOT) 2> $(LOGS_DIR)/clone_hdk.log && \
		(cd $(_HDK_ROOT) && git checkout v1.4.15 >> $(LOGS_DIR)/clone_hdk.log ) \
	fi

$(_CL_DIR):
	@mkdir -p $(_CL_DIR)

overlay:
	@echo "Assemble the Metal FS overlay from User IP"

model: $(_HDK_ROOT) $(_CL_DIR)
	@echo "                        Creating simulation model..."
	@$(METAL_ROOT)/targets/AWS-F1/_internal/scripts/build_model

sim:
	@echo "Start simulation"

image:
	@echo "Create FPGA image"

help: $(_HDK_ROOT)
	@echo "(optional)"

clean:
	@echo "clean"


# $(_HDK_ROOT)/snap_env.sh: $(_HDK_ROOT)
# 	@mkdir -p $(LOGS_DIR)
# 	@echo "                        Configuring SNAP..."
# 	@$(METAL_ROOT)/targets/$(METAL_TARGET)/configure > $(LOGS_DIR)/configure_snap.log 2>&1

# $(snap_targets): $(_HDK_ROOT)/snap_env.sh $(operators) $(IMAGE_TARGET)
# 	@make -C $(_HDK_ROOT) -s $@

# sim:
# 	@$(METAL_ROOT)/targets/SNAP/_internal/scripts/sim

test_target:
	@make -C $(_ACTION_ROOT)/hw test
