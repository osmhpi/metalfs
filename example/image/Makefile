ifndef SNAP_ROOT
$(info You have specified a wrong $$SNAP_ROOT.)
$(error Please make sure that $$SNAP_ROOT is set up correctly.)
endif

IMAGE_JSON = $(PWD)/image.json
HLS_OPERATOR_PATH = $(PWD)/src
IMAGE_TARGET = $(PWD)/operators

snap_targets = hw_project model sim sim_screen image help

all: help

$(snap_targets):
	@ \
	IMAGE_JSON=$(IMAGE_JSON) \
	HLS_OPERATOR_PATH=$(HLS_OPERATOR_PATH) \
	IMAGE_TARGET=$(IMAGE_TARGET) \
	make -C $(SNAP_ROOT) -s $@
