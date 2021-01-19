# export SNAP_TARGET 	       = $(shell basename $(METAL_TARGET))
export LOGS_DIR            = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/logs
export OPERATORS_BUILD_DIR = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/operators

# Define underscored variables as not to interfere with externally set values
export _PSLSE_ROOT  = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/pslse
export _SNAP_ROOT   = $(IMAGE_BUILD_DIR)/$(METAL_TARGET)/snap
export _ACTION_ROOT = $(METAL_ROOT)/targets/SNAP/_internal/action

CONFIG_FILE  = $(_SNAP_ROOT)/.snap_config
ifeq ($(FPGACHIP),)
export FPGACHIP  = $(shell grep FPGACHIP $(CONFIG_FILE) | cut -d = -f 2 | tr -d '"')
endif

snap_targets = snap_config clean hw_project model image help

overlay: hw_project

$(_PSLSE_ROOT):
	@mkdir -p $(LOGS_DIR)
	@if [ ! -e $(_PSLSE_ROOT) ] ; then \
		echo "                        Cloning PSLSE..." ; \
		git clone https://github.com/ibm-capi/pslse $(_PSLSE_ROOT) 2> $(LOGS_DIR)/clone_pslse.log && \
		(cd $(_PSLSE_ROOT) && git checkout v4.1 2>> $(LOGS_DIR)/clone_pslse.log ) \
	fi

$(_SNAP_ROOT): $(_PSLSE_ROOT)
	@mkdir -p $(LOGS_DIR)
	@if [ ! -e $(_SNAP_ROOT) ] ; then \
		echo "                        Cloning SNAP..." ; \
		git clone https://github.com/metalfs/snap $(_SNAP_ROOT) 2> $(LOGS_DIR)/clone_snap.log && \
		(cd $(_SNAP_ROOT) && git checkout 3238762 2>> $(LOGS_DIR)/clone_snap.log ) \
	fi

$(_SNAP_ROOT)/snap_env.sh: $(_SNAP_ROOT)
	@mkdir -p $(LOGS_DIR)
	@echo "                        Configuring SNAP..."
	@$(METAL_ROOT)/targets/$(METAL_TARGET)/configure > $(LOGS_DIR)/configure_snap.log 2>&1

$(snap_targets): $(_SNAP_ROOT)/snap_env.sh $(operators) $(IMAGE_TARGET)
	@make -C $(_SNAP_ROOT) -s $@

sim:
	@$(METAL_ROOT)/targets/SNAP/_internal/scripts/sim

test_target:
	@make -C $(_ACTION_ROOT)/hw test
