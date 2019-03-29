SOLUTION_NAME ?= hls_action_sln
SOLUTION_DIR ?= hls_action_sln

srcs += axi_switch.cpp \
	mtl_extmap.cpp \
	mtl_file.cpp \
	mtl_jobstruct.cpp \
	mtl_op_mem.cpp \
	mtl_op_passthrough.cpp \
	mtl_op_image.cpp \
	mtl_op_change_case.cpp \
	hls_metalfpga.cpp

WRAPPER ?= hls_action

include $(SNAP_ROOT)/actions/hls.mk
include ../hls.mk
