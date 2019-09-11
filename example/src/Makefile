ifndef SNAP_ROOT
$(info You have specified a wrong $$SNAP_ROOT.)
$(error Please make sure that $$SNAP_ROOT is set up correctly.)
endif

export IMAGE_JSON        = $(PWD)/image.json
export IMAGE_TARGET      = $(PWD)/image_compiled.json
export METAL_ROOT        = $(PWD)/../..
export METAL_TARGET      = $(shell jq -r .target $(IMAGE_JSON))

snap_targets = snap_config clean clean_config hw_project model image help

all: help

clean: clean_operators

clean_operators:
	@for dir in ./src/hls_*/; do make -C $$dir -s clean; done

$(SNAP_ROOT)/snap_env.sh:
	@$(METAL_ROOT)/targets/$(METAL_TARGET)/configure

$(snap_targets): $(SNAP_ROOT)/snap_env.sh
	@make -C $(SNAP_ROOT) -s $@

sim:
	@$(METAL_ROOT)/targets/$(METAL_TARGET)/sim
