ifndef SNAP_ROOT
$(info You have specified a wrong $$SNAP_ROOT.)
$(error Please make sure that $$SNAP_ROOT is set up correctly.)
endif

IMAGE_JSON        = $(PWD)/image.json
IMAGE_TARGET      = $(PWD)/image_compiled.json
METAL_ROOT        = $(PWD)/../..
METAL_TARGET      = $(shell jq -r .target $(IMAGE_JSON))

snap_targets = snap_config clean clean_config hw_project model sim sim_tmux image help

all: help

clean: clean_operators

clean_operators:
	@for dir in ./src/hls_*/; do make -C $$dir -s clean; done

$(SNAP_ROOT)/snap_env.sh:
	@ \
	IMAGE_JSON=$(IMAGE_JSON) \
	IMAGE_TARGET=$(IMAGE_TARGET) \
	METAL_ROOT=$(METAL_ROOT) \
	$(METAL_ROOT)/targets/$(METAL_TARGET)/configure

$(snap_targets): $(SNAP_ROOT)/snap_env.sh
	@ \
	IMAGE_JSON=$(IMAGE_JSON) \
	IMAGE_TARGET=$(IMAGE_TARGET) \
	METAL_ROOT=$(METAL_ROOT) \
	make -C $(SNAP_ROOT) -s $@
