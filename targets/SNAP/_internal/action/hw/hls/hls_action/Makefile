WRAPPER       ?= hls_action
STREAM_BYTES  ?= 8
BUILD_DIR     = $(ACTION_BUILDROOT)/hls_action

HLS_CFLAGS    += \
	-I$(SNAP_ROOT)/actions/include \
	-I$(SNAP_ROOT)/software/include \
	-I$(METAL_ROOT)/targets/SNAP/_internal/include

srcs += \
	axi_switch.cpp \
	axi_perfmon.cpp \
	mtl_extmap.cpp \
	mtl_jobs.cpp \
	mtl_op_mem.cpp \
	operators.cpp \
	hls_action.cpp

testbench_srcs += \
	transfer_to_stream_test.cpp \
	transfer_from_stream_test.cpp

include $(METAL_ROOT)/buildpacks/hls/hls.mk
HLS_ACTION_CLOCK = 3.5
