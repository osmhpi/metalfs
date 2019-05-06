SOLUTION_NAME ?= hls_action_sln
SOLUTION_DIR ?= hls_action_sln

srcs += \
	axi_switch.h \
	axi_switch.cpp \
	axi_perfmon.h \
	axi_perfmon.cpp \
	mtl_extmap.h \
	mtl_extmap.cpp \
	mtl_file.h \
	mtl_file.cpp \
	mtl_jobs.h \
	mtl_jobs.cpp \
	mtl_jobstruct.h \
	mtl_jobstruct.cpp \
	mtl_op_file.h \
	mtl_op_file.cpp \
	mtl_op_mem.h \
	mtl_op_mem.cpp \
	operators.h \
	operators.cpp \
	hls_action.cpp

WRAPPER ?= hls_action

include $(SNAP_ROOT)/actions/hls.mk
include ../hls.mk
