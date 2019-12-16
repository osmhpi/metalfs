WRAPPER       ?= hls_streamgen
STREAM_BYTES  ?= 8
BUILD_DIR     = $(ACTION_BUILDROOT)/hls_streamgen

HLS_CFLAGS    += \
	-I$(SNAP_ROOT)/actions/include \
	-I$(SNAP_ROOT)/software/include

srcs += hls_streamgen.cpp

include $(METAL_ROOT)/buildpacks/hls/hls.mk
