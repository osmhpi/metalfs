ifndef SNAP_ROOT
$(info You have specified a wrong $$SNAP_ROOT.)
$(error Please make sure that $$SNAP_ROOT is set up correctly.)
endif

ifndef IMAGE_JSON
$(info There is no image manifest path defined.)
$(error Please make sure that $$IMAGE_JSON is set up correctly.)
endif

vhd_srcs=$(wildcard *.vhd_source)
vhd_files=$(vhd_srcs:.vhd_source=.vhd)

ACTION_ROOT ?= $(PWD)/..
LOGS_DIR ?= $(PWD)/../logs
CONFIG_FILE = $(SNAP_ROOT)/.snap_config

hls_components  = $(shell find hls/ -maxdepth 1 -type d -name 'hls_*')
metal_operators = $(shell ./resolve_operators $(IMAGE_JSON) | cut -f 2 | uniq)

ADDITIONAL_HLS_CFLAGS += -DSTREAM_BYTES=$(shell jq -r .stream_bytes $(IMAGE_JSON))

ifeq ($(DDRI_USED),)
	export DDRI_USED = $(shell grep DDRI_USED $(CONFIG_FILE) | cut -d = -f 2 | tr -d '"')
endif
ifeq ($(NVME_USED),)
	export NVME_USED = $(shell grep NVME_USED $(CONFIG_FILE) | cut -d = -f 2 | tr -d '"')
endif
ifeq ($(FPGACHIP),)
	export FPGACHIP  = $(shell grep FPGACHIP $(CONFIG_FILE) | cut -d = -f 2 | tr -d '"')
endif

ifeq ($(DDRI_USED),TRUE)
	DDRI_FILTER = "\-\- only for DDRI_USED!=TRUE"
else
	DDRI_FILTER = "\-\- only for DDRI_USED=TRUE"
endif

ifeq ($(NVME_USED),TRUE)
	NVME_FILTER = "\-\- only for NVME_USED!=TRUE"
else
	NVME_FILTER = "\-\- only for NVME_USED=TRUE"
endif

.PHONY: clean vhdl $(hls_components) $(metal_operators) block_design image_info nvme_arbiter

all: vhd_files block_design

$(IMAGE_TARGET): $(IMAGE_JSON)
	@./generate_image $(IMAGE_JSON) $(IMAGE_TARGET)

block_design: hls operators nvme_arbiter image_info $(LOGS_DIR)
	@echo "                        Generating Block Design"
	@make -C $(ACTION_ROOT)/ip/block_design ip

image_info: $(IMAGE_TARGET) $(LOGS_DIR)
	@echo "                        Generating Image Info"
	@make -C $(ACTION_ROOT)/ip/image_info ip

nvme_arbiter: $(LOGS_DIR)
	@echo "                        Generating NVMe Arbiter"
	@make -C $(ACTION_ROOT)/ip/nvme_arbiter ip

$(LOGS_DIR):
	@mkdir -p $(LOGS_DIR)

hls: $(hls_components)

$(hls_components): $(LOGS_DIR)
	@echo "                        Generating IP from HLS component $(@F)"
	@echo "Calling make ADDITIONAL_HLS_CFLAGS='$(ADDITIONAL_HLS_CFLAGS)' -C $@ ip" > $(LOGS_DIR)/$(@F)_make.log
	@make ADDITIONAL_HLS_CFLAGS='$(ADDITIONAL_HLS_CFLAGS)' -C $@ ip >> $(LOGS_DIR)/$(@F)_make.log; hls_ret=$$?; \
	if [ $${hls_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/$(@F)_make.log"; exit -1; \
	fi

operators: $(metal_operators)

$(metal_operators): $(LOGS_DIR)
	@echo "                        Generating IP from Metal FS Operator $(@F)"
	@echo "Calling make ADDITIONAL_HLS_CFLAGS='$(ADDITIONAL_HLS_CFLAGS)' -C $@ ip" > $(LOGS_DIR)/$(@F)_make.log
	@make ADDITIONAL_HLS_CFLAGS='$(ADDITIONAL_HLS_CFLAGS)' -C $@ ip >> $(LOGS_DIR)/$(@F)_make.log; hls_ret=$$?; \
	if [ $${hls_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/$(@F)_make.log"; exit -1; \
	fi

vhd_files: $(vhd_files)

$(vhd_files): $(vhd_srcs)
	@echo "                        Generating $@"
	@grep -v $(DDRI_FILTER) $< | grep -v $(NVME_FILTER) > $@

clean:
	@$(RM) $(vhd_files)

	@for hls_dir in ./hls/hls_*/; do make -C $$hls_dir clean; done
	@for ip_dir in ../ip/*; do make -C $$ip_dir clean; done

vhdl:
