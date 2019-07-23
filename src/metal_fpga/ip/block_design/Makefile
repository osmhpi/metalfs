sources += \
	create_action_ip.tcl \
	create_bd.tcl \
	instantiate_operators.tcl

ip: ip_user_files/component.xml

ip_user_files/component.xml: $(sources)
	@DDRI_USED=$(DDRI_USED) NVME_USED=$(NVME_USED) vivado -quiet -mode batch -source create_action_ip.tcl -notrace -nojournal -tclargs $(ACTION_ROOT) $(FPGACHIP)
