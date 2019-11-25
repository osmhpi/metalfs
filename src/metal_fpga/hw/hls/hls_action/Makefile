SOLUTION_NAME ?= hls_action_sln
SOLUTION_DIR ?= hls_action_sln

HLS_CFLAGS += -std=c++11

srcs += \
	axi_switch.cpp \
	axi_perfmon.cpp \
	mtl_extmap.cpp \
	mtl_jobs.cpp \
	mtl_op_mem.cpp \
	operators.cpp \
	hls_action.cpp

testbench_srcs += \
	transfer_test.cpp

WRAPPER ?= hls_action

include $(SNAP_ROOT)/actions/hls.mk
include ../hls.mk
