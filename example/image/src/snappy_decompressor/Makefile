OPERATOR_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

sources += \
	create_ip.tcl

ip: ip_user_files/component.xml

ip_user_files/component.xml: $(sources)
	@vivado -quiet -mode batch -source create_ip.tcl -notrace -nojournal -tclargs $(OPERATOR_ROOT) $(FPGACHIP)
