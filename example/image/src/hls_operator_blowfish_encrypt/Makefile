SOLUTION_NAME ?= hls_operator_blowfish_encrypt_sln
SOLUTION_DIR ?= hls_operator_blowfish_encrypt_sln

srcs += \
	hls_operator_blowfish_encrypt.cpp \
	hls_operator_blowfish_encrypt.h

WRAPPER ?= hls_operator_blowfish_encrypt

include $(SNAP_ROOT)/actions/hls.mk
include ../../../../src/metal_fpga/hw/hls/hls.mk
