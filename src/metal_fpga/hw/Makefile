ifndef SNAP_ROOT
$(info You have specified a wrong $$SNAP_ROOT.)
$(error Please make sure that $$SNAP_ROOT is set up correctly.)
endif

vhd_srcs=$(wildcard *.vhd_source)
vhd_files=$(vhd_srcs:.vhd_source=.vhd)

hls_components=$(shell find hls/ -maxdepth 1 -type d -name 'hls_*')
hls_ip=$(hls_components:=/hls_impl_ip)

ACTION_ROOT ?= $(PWD)/..
LOGS_DIR ?= $(PWD)/../logs
CONFIG_FILE = $(SNAP_ROOT)/.snap_config

ifeq ($(DDRI_USED),)
	DDRI_USED := $(shell grep DDRI_USED $(CONFIG_FILE) | cut -d = -f 2 | tr -d '"')
endif
ifeq ($(NVME_USED),)
	NVME_USED := $(shell grep NVME_USED $(CONFIG_FILE) | cut -d = -f 2 | tr -d '"')
endif
ifeq ($(FPGACHIP),)
	FPGACHIP := $(shell grep FPGACHIP $(CONFIG_FILE) | cut -d = -f 2 | tr -d '"')
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

.PHONY: config clean vhdl $(hls_components)

all: vhd_files hls $(ACTION_ROOT)/ip/action_ip_prj/action_ip_prj.srcs/sources_1/bd/bd_action/hdl/bd_action_wrapper.vhd

$(ACTION_ROOT)/ip/action_ip_prj/action_ip_prj.srcs/sources_1/bd/bd_action/hdl/bd_action_wrapper.vhd: $(hls_ip)
	@echo "                        Generating Block Design"
	@DDRI_USED=$(DDRI_USED) NVME_USED=$(NVME_USED) vivado -quiet -mode batch -source $(ACTION_ROOT)/ip/create_action_ip.tcl -notrace -nojournal -tclargs $(ACTION_ROOT) $(FPGACHIP)

hls: $(hls_components)

$(hls_components):
	@mkdir -p $(LOGS_DIR)
	@echo "                        Generating IP from HLS component $(@F)"
	@echo "Calling make DDRI_USED=$(DDRI_USED) NVME_USED=$(NVME_USED) -C $@ ip" > $(LOGS_DIR)/$(@F)_make.log
	@make DDRI_USED=$(DDRI_USED) NVME_USED=$(NVME_USED) -C $@ ip >> $(LOGS_DIR)/$(@F)_make.log; hls_ret=$$?; \
	if [ $${hls_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/$(@F)_make.log"; exit -1; \
	fi

vhd_files: $(vhd_files)

$(vhd_files): $(vhd_srcs) .snap_config
	@echo "                        Generating $@"
	@grep -v $(DDRI_FILTER) $< | grep -v $(NVME_FILTER) > $@

clean:
	@$(RM) $(ACTION_ROOT)/ip/*.log
	@$(RM) $(ACTION_ROOT)/ip/*.jou
	@$(RM) -fr $(ACTION_ROOT)/ip/action_ip_prj
	@$(RM) -fr $(ACTION_ROOT)/ip/ip_user_files
	@$(RM) $(vhd_files)

	@for hls_dir in ./hls/hls_*/; do make -C $$hls_dir clean; done

vhdl:

.snap_config: FORCE
	@if [ -n .snap_config ]; then touch .snap_config; fi
	@if [ $$(echo DDRI_USED=$(DDRI_USED) NVME_USED=$(NVME_USED) | comm -3 .snap_config - | wc -c) -gt 0 ]; then echo DDRI_USED=$(DDRI_USED) NVME_USED=$(NVME_USED) > .snap_config; fi

FORCE:
