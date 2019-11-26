WRAPPER = ImageInfo

srcs += \
	config_store.vhd \
	image_info.vhd

info: $(IMAGE_TARGET)
	@jq -c . $(IMAGE_TARGET) | xxd -p | tr -d '\n' > info

image_info.vhd: image_info.vhd_source info
	@./format_vhd $< info $@

include $(METAL_ROOT)/buildpacks/hdl/hdl.mk

clean: clean_imageinfo
clean_imageinfo:
	@$(RM) info
	@$(RM) image_info.vhd