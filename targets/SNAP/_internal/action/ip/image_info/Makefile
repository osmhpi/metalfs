BUILD_DIR=$(ACTION_BUILDROOT)/image_info

WRAPPER = ImageInfo

srcs += \
	config_store.vhd \
	$(BUILD_DIR)/image_info.vhd

$(BUILD_DIR)/info: $(IMAGE_TARGET)
	mkdir -p $(BUILD_DIR)
	@jq -c . $(IMAGE_TARGET) | xxd -p | tr -d '\n' > $@

$(BUILD_DIR)/image_info.vhd: image_info.vhd_source $(BUILD_DIR)/info
	@./format_vhd $< $(BUILD_DIR)/info $@

include $(METAL_ROOT)/buildpacks/hdl/hdl.mk
