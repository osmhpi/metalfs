export LOGS_DIR            = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/logs
export OPERATORS_BUILD_DIR = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/operators

# Define underscored variables as not to interfere with externally set values
export _OCSE_ROOT    = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/ocse
export _OCACCEL_ROOT = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/oc-accel
export _ACTION_ROOT  = $(METAL_ROOT)/targets/OCAccel/_internal/action

OCSE_REPO       = https://github.com/OpenCAPI/ocse
OCSE_RELEASE    = 26044e5
OCACCEL_REPO    = https://github.com/metalfs/oc-accel
OCACCEL_RELEASE = metalfs

CONFIG_FILE  = $(_OCACCEL_ROOT)/.snap_config
ifeq ($(FPGACHIP),)
export FPGACHIP  = $(shell if [ -f $(CONFIG_FILE) ]; then grep FPGACHIP $(CONFIG_FILE) | cut -d = -f 2 | tr -d '"'; fi)
endif

ocaccel_targets = snap_config clean hw_project model image help

overlay: hw_project

$(_OCSE_ROOT):
	@mkdir -p $(LOGS_DIR)
	@if [ ! -e $(_OCSE_ROOT) ] ; then \
		echo "                        Cloning OCSE..." ; \
		git clone $(OCSE_REPO) $(_OCSE_ROOT) 2> $(LOGS_DIR)/clone_ocse.log && \
		(cd $(_OCSE_ROOT) && git checkout $(OCSE_RELEASE) 2>> $(LOGS_DIR)/clone_ocse.log ) \
	fi

$(_OCACCEL_ROOT): $(_OCSE_ROOT)
	@mkdir -p $(LOGS_DIR)
	@if [ ! -e $(_OCACCEL_ROOT) ] ; then \
		echo "                        Cloning OCAccel..." ; \
		git clone $(OCACCEL_REPO) $(_OCACCEL_ROOT) 2> $(LOGS_DIR)/clone_ocaccel.log && \
		(cd $(_OCACCEL_ROOT) && git checkout $(OCACCEL_RELEASE) 2>> $(LOGS_DIR)/clone_ocaccel.log ) \
	fi

$(_OCACCEL_ROOT)/snap_env.sh: $(_OCACCEL_ROOT)
	@mkdir -p $(LOGS_DIR)
	@echo "                        Configuring OCAccel..."
	@$(METAL_ROOT)/targets/$(METAL_TARGET)/configure > $(LOGS_DIR)/configure_ocaccel.log 2>&1


$(ocaccel_targets): $(_OCACCEL_ROOT)/snap_env.sh $(operators) $(IMAGE_TARGET)
	@make -C $(_OCACCEL_ROOT) -s $@

sim:
	@$(METAL_ROOT)/targets/OCAccel/_internal/scripts/sim

test_target:
	@make -C $(_ACTION_ROOT)/hw test
