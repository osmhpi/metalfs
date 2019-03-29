#
# Generate HDL version of the HLS sources
#
# The generated HDL depends on the chip which is used and
# therefore must match what is being used to build the
# toplevel SNAP bitstream.
#
# FIXME Pass part_number and other parameters from toplevel
#      build-system as required.
#

# Finding $SNAP_ROOT
ifndef SNAP_ROOT
# check if we are in sw folder of an action (three directories below snap root)
ifneq ("$(wildcard ../../../ActionTypes.md)","")
SNAP_ROOT=$(abspath ../../../)
else
$(info You are not building your software from the default directory (/path/to/snap/actions/<action_name>/sw) or specified a wrong $$SNAP_ROOT.)
$(error Please make sure that $$SNAP_ROOT is set up correctly.)
endif
endif

# This is solution specific. Check if we can replace this by generics too.

snap_fs: snap_fs.o

projs += snap_fs

# If you have the host code outside of the default snap directory structure,
# change to /path/to/snap/actions/software.mk
include $(SNAP_ROOT)/actions/software.mk

CFLAGS += -I../hw/hls/include
