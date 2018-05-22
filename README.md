# metal_fs

## How to build

### Hardware
 - Set up a working CAPI SNAP build environment using the [metalfs/snap](https://github.com/metalfs/snap) master branch
 - Configure SNAP to use an external HLS action, rooted in `metal_fs/src/metal_fpga`, with SDRAM and NVMe enabled
 - Run `make model` / `make image` as usual

### Software
 - CMake is required to build the software, as well as FUSE (`libfuse-dev` on Ubuntu), LMDB (`liblmdb-dev`) and libcxl (part of [PSLSE](https://github.com/ibm-capi/pslse) when simulating)
 - Create a build directory: `mkdir build && cd build`
 - Run CMake, explicitly specifying paths to SNAP and libcxl if necessary (the other dependencies should be automatically resolved):
 ```
  cmake .. \
    -DSNAP_INCLUDE_DIR=~/snap/software/include \
    -DSNAP_LIBRARY=~/snap/software/lib/libsnap.a \
    -DCXL_INCLUDE_DIR=~/pslse/libcxl \
    -DCXL_LIBRARY=~/pslse/libcxl/libcxl.a
  ```
 - Run `make` to build all libraries and executables


## How to run
 - Enter an environment where the SNAP action can be attached (i.e. either in simulation or on the POWER machine)
 - Move inside the `build` directory, and `mkdir metadata_store` (this is where the filesystem metadata will reside)
 - Create a mount point (e.g. `mkdir /tmp/metal_fs`)
 - Launch the user-mode file system driver: `./metal_fs -s -f /tmp/metal_fs`
