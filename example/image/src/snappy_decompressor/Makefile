OPERATOR_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

sources += \
	create_ip.tcl \
	SnappyWrapper.vhd \
	fifo.v \
	preparser.v \
	control.v \
	fifo_parser_copy.v \
	queue_token.v \
	copyread_selector.v   \
	fifo_parser_lit.v   \
	ram_block.v \
	copytoken_selector.v  \
	io_control.v \
	ram_module.v \
	data_out.v \
	lit_selector.v \
	select.v \
	decompressor.v \
	parser.v \
	distributor.v \
	parser_sub.v

ip: hls_impl_ip/component.xml

hls_impl_ip/component.xml: $(sources)
	@vivado -quiet -mode batch -source create_ip.tcl -nolog -notrace -nojournal -tclargs $(OPERATOR_ROOT) $(FPGACHIP)
