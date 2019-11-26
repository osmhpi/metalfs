export SNAP_TARGET = $(shell basename $(METAL_TARGET))

# Define underscored variables as not to interfere with externally set values
export _PSLSE_ROOT  = $(METAL_ROOT)/targets/SNAP/build/$(SNAP_TARGET)/pslse
export _SNAP_ROOT   = $(METAL_ROOT)/targets/SNAP/build/$(SNAP_TARGET)/snap
export _ACTION_ROOT = $(METAL_ROOT)/targets/SNAP/action

snap_targets = snap_config clean hw_project model sim sim_screen image help

overlay: hw_project

$(_PSLSE_ROOT):
	@git clone https://github.com/ibm-capi/pslse $(_PSLSE_ROOT)

$(_SNAP_ROOT): $(_PSLSE_ROOT)
	@git clone https://github.com/metalfs/snap $(_SNAP_ROOT)

$(_ACTION_ROOT):
	@mkdir -p $(_ACTION_ROOT)
	# @cp -r $(METAL_ROOT)/targets/$(METAL_TARGET)/../action_template/* $(_ACTION_ROOT)

$(_SNAP_ROOT)/snap_env.sh: $(_SNAP_ROOT) $(_ACTION_ROOT)
	@$(METAL_ROOT)/targets/$(METAL_TARGET)/configure

$(snap_targets): $(_SNAP_ROOT)/snap_env.sh
	@make -C $(_SNAP_ROOT) -s $@
