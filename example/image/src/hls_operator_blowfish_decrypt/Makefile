SOLUTION_NAME ?= hls_operator_blowfish_decrypt_sln
SOLUTION_DIR ?= hls_operator_blowfish_decrypt_sln

HLS_CFLAGS=-I../../../../src/metal_fpga/hw/hls/include

srcs += \
	hls_operator_blowfish_decrypt.cpp

WRAPPER ?= hls_operator_blowfish_decrypt

include $(SNAP_ROOT)/actions/hls.mk
include ../../../../src/metal_fpga/hw/hls/hls.mk
