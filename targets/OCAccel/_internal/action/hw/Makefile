export ACTION_BUILDROOT=$(IMAGE_BUILD_DIR)/$(METAL_TARGET)/action

ACTION_ROOT ?= $(PWD)/..
CONFIG_FILE  = $(_OCACCEL_ROOT)/.snap_config

hls_components  = $(shell find hls/ -maxdepth 1 -type d -name 'hls_*')

.PHONY: clean vhdl $(hls_components) block_design image_info nvme_arbiter

all: block_design $(ACTION_BUILDROOT)/action_wrapper.v

$(LOGS_DIR):
	@mkdir -p $(LOGS_DIR)

block_design: hls nvme_arbiter image_info $(LOGS_DIR)
	@echo "                        Generating Block Design"
	@make -C $(ACTION_ROOT)/ip/block_design ip > $(LOGS_DIR)/$(@F)_make.log; viv_ret=$$?; \
	if [ $${viv_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/$(@F)_make.log"; exit -1; \
	fi

image_info: $(IMAGE_TARGET) $(LOGS_DIR)
	@echo "                        Generating Image Info"
	@make -C $(ACTION_ROOT)/ip/image_info ip > $(LOGS_DIR)/$(@F)_make.log; viv_ret=$$?; \
	if [ $${viv_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/$(@F)_make.log"; exit -1; \
	fi

nvme_arbiter: $(LOGS_DIR)
	@echo "                        Generating NVMe Arbiter"
	@make -C $(ACTION_ROOT)/ip/nvme_arbiter ip > $(LOGS_DIR)/$(@F)_make.log; viv_ret=$$?; \
	if [ $${viv_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/$(@F)_make.log"; exit -1; \
	fi

hls: $(hls_components)

$(hls_components): $(LOGS_DIR)
	@echo "                        Generating IP from HLS component $(@F)"
	@echo "Calling make -C $@ ip" > $(LOGS_DIR)/$(@F)_make.log
	@make -C $@ ip >> $(LOGS_DIR)/$(@F)_make.log; hls_ret=$$?; \
	if [ $${hls_ret} -ne 0 ]; then \
		echo "                        Error: please look into $(LOGS_DIR)/$(@F)_make.log"; exit -1; \
	fi

test: $(LOGS_DIR)
	@for hls_dir in ./hls/hls_*/; do \
	make -C $$hls_dir test; hls_ret=$$?; \
	if [ $${hls_ret} -ne 0 ]; then \
		echo "                        Error: Test of $$(basename $$hls_dir) failed"; exit -1; \
	fi \
	done


$(ACTION_BUILDROOT):
	@mkdir -p $(ACTION_BUILDROOT)

$(ACTION_BUILDROOT)/action_wrapper.v: $(ACTION_BUILDROOT) action_wrapper.v
	@cp action_wrapper.v $(ACTION_BUILDROOT)/action_wrapper.v

clean:
	# @$(RM) $(vhd_files)

	@for hls_dir in ./hls/hls_*/; do make -C $$hls_dir clean; done
	@for ip_dir in ../ip/*; do make -C $$ip_dir clean; done

vhdl:
