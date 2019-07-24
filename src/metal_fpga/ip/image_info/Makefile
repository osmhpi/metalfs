sources += \
	create_ip.tcl \
	config_store.vhd \
	image_info.vhd

ip: ip_user_files/component.xml

info: $(IMAGE_TARGET)
	@jq -c . $(IMAGE_TARGET) | xxd -p | tr -d '\n' > info

image_info.vhd: image_info.vhd_source info
	@./format_vhd $< info $@

ip_user_files/component.xml: $(sources)
	@vivado -quiet -mode batch -source create_ip.tcl -notrace -nojournal -tclargs $(ACTION_ROOT) $(FPGACHIP)

clean:
	@$(RM) info
	@$(RM) image_info.vhd
	@$(RM) *.log
	@$(RM) *.jou
	@$(RM) -fr managed_ip_project
	@$(RM) -fr ip_user_files