sources += \
	create_ip.tcl \
	NvmeController.vhd \
	NvmeControllerWrapper.vhd \
	fosi_ctrl.vhd \
	fosi_nvme.vhd \
	fosi_util.vhd

ip: ip_user_files/component.xml

ip_user_files/component.xml: $(sources)
	@vivado -quiet -mode batch -source create_ip.tcl -notrace -nojournal -tclargs $(ACTION_ROOT) $(FPGACHIP)
