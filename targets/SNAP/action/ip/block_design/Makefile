IP_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

sources += \
	create_action_ip.tcl \
	create_bd.tcl \
	instantiate_operators.tcl

ip: ip_user_files/component.xml

ip_user_files/component.xml: $(sources)
	@vivado -quiet -mode batch -source create_action_ip.tcl -nolog -notrace -nojournal -tclargs $(IP_ROOT) $(FPGACHIP) $(LOGS_DIR)

clean:
	@$(RM) *.log
	@$(RM) *.jou
	@$(RM) -fr action_ip_prj
	@$(RM) -fr ip_user_files