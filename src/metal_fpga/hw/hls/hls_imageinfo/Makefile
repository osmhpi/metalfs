ifndef IMAGE_TARGET
$(info There is no image manifest path defined.)
$(error Please make sure that $$IMAGE_TARGET is set up correctly.)
endif

SOLUTION_NAME ?= hls_imageinfo_sln
SOLUTION_DIR ?= hls_imageinfo_sln

HLS_CFLAGS += -std=c++11

srcs += hls_imageinfo.cpp

WRAPPER ?= hls_imageinfo

include $(SNAP_ROOT)/actions/hls.mk
include ../hls.mk

info: $(IMAGE_TARGET)
	@jq -c . $(IMAGE_TARGET) > info

info.hpp: info
	@xxd -i info > info.hpp

$(srcs): info.hpp
