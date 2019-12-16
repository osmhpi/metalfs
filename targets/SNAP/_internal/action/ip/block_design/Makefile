IP_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BD_BUILD_ROOT=$(ACTION_BUILDROOT)/block_design

$(BD_BUILD_ROOT):
	@mkdir -p $(BD_BUILD_ROOT)

sources += \
	create_action_ip.tcl \
	create_bd.tcl \
	instantiate_operators.tcl

ip: $(BD_BUILD_ROOT)/ip_user_files/component.xml

$(BD_BUILD_ROOT)/ip_user_files/component.xml: $(sources) $(BD_BUILD_ROOT)
	@cd $(BD_BUILD_ROOT) && vivado -quiet -mode batch -source $(IP_ROOT)/create_action_ip.tcl -nolog -notrace -nojournal -tclargs $(IP_ROOT) $(ACTION_BUILDROOT) $(FPGACHIP) $(LOGS_DIR)

clean:
	@$(RM) -fr $(BD_BUILD_ROOT)
