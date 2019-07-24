IP_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

sources += \
	create_ip.tcl \
	NvmeController.vhd \
	NvmeControllerWrapper.vhd \
	fosi_ctrl.vhd \
	fosi_nvme.vhd \
	fosi_util.vhd

ip: ip_user_files/component.xml

ip_user_files/component.xml: $(sources)
	@vivado -quiet -mode batch -source create_ip.tcl -nolog -notrace -nojournal -tclargs $(IP_ROOT) $(FPGACHIP) $(LOGS_DIR)

clean:
	@$(RM) *.log
	@$(RM) *.jou
	@$(RM) -fr managed_ip_project
	@$(RM) -fr ip_user_files
