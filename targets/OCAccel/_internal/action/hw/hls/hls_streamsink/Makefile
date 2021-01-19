WRAPPER       ?= hls_streamsink
STREAM_BYTES  ?= 8
BUILD_DIR     = $(ACTION_BUILDROOT)/hls_streamsink

HLS_CFLAGS    += \
	-I$(_OCACCEL_ROOT)/actions/include \
	-I$(_OCACCEL_ROOT)/software/include

srcs += hls_streamsink.cpp

include $(METAL_ROOT)/buildpacks/hls/hls.mk
