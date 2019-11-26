WRAPPER       ?= hls_streamsink
STREAM_BYTES  ?= 8
BUILD_DIR     = $(METAL_ROOT)/targets/SNAP/build/$(SNAP_TARGET)/ip/hls_streamsink

HLS_CFLAGS    += \
	-I$(SNAP_ROOT)/actions/include \
	-I$(SNAP_ROOT)/software/include

srcs += hls_streamsink.cpp

include $(METAL_ROOT)/buildpacks/hls/hls.mk
