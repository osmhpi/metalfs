IP_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

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
	@vivado -quiet -mode batch -source create_ip.tcl -nolog -notrace -nojournal -tclargs $(IP_ROOT) $(FPGACHIP) $(LOGS_DIR)

clean:
	@$(RM) info
	@$(RM) image_info.vhd
	@$(RM) *.log
	@$(RM) *.jou
	@$(RM) -fr managed_ip_project
	@$(RM) -fr ip_user_files