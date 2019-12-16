export SNAP_TARGET = $(shell basename $(METAL_TARGET))

# Define underscored variables as not to interfere with externally set values
export _PSLSE_ROOT  = $(BUILD_DIR)/$(SNAP_TARGET)/pslse
export _SNAP_ROOT   = $(BUILD_DIR)/$(SNAP_TARGET)/snap
export _ACTION_ROOT = $(METAL_ROOT)/targets/SNAP/action
export LOGS_DIR     = $(BUILD_DIR)/$(SNAP_TARGET)/logs

snap_targets = snap_config clean hw_project model sim sim_screen image help

overlay: hw_project

$(_PSLSE_ROOT):
	@if [ ! -e $(_PSLSE_ROOT) ] ; then \
		git clone https://github.com/ibm-capi/pslse $(_PSLSE_ROOT) && \
		(cd $(_PSLSE_ROOT) && git checkout v4.1) \
	fi

$(_SNAP_ROOT): $(_PSLSE_ROOT)
	@if [ ! -e $(_SNAP_ROOT) ] ; then \
		git clone https://github.com/metalfs/snap $(_SNAP_ROOT) && \
		(cd $(_SNAP_ROOT) && git checkout 46c239d) \
	fi

$(_SNAP_ROOT)/snap_env.sh: $(_SNAP_ROOT)
	@$(METAL_ROOT)/targets/$(METAL_TARGET)/configure

$(snap_targets): $(_SNAP_ROOT)/snap_env.sh $(operators) $(IMAGE_TARGET)
	@make -C $(_SNAP_ROOT) -s $@