WRAPPER       ?= hls_streamgen
STREAM_BYTES  ?= 8
BUILD_DIR     = $(ACTION_BUILDROOT)/hls_streamgen

HLS_CFLAGS    += \
	-I$(_SNAP_ROOT)/actions/include \
	-I$(_SNAP_ROOT)/software/include

srcs += hls_streamgen.cpp

include $(METAL_ROOT)/buildpacks/hls/hls.mk
