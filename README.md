# metal_fs

## How to build

### Hardware
 - Set up a working CAPI SNAP build environment using the [metalfs/snap](https://github.com/metalfs/snap) master branch
 - Configure SNAP to use an external HLS action, rooted in `metal_fs/src/metal_fpga`, with SDRAM and NVMe enabled
 - Run `make model` / `make image` as usual

### Software
 - CMake is required to build the software, as well as FUSE (`libfuse-dev` on Ubuntu), LMDB (`liblmdb-dev`) and libcxl (part of [PSLSE](https://github.com/ibm-capi/pslse) when simulating)
 - Create a build directory: `mkdir build && cd build`
 - Specify environment variables pointing to SNAP (and if necessary PSLSE). Make sure that the libcxl.a and libsnap.a libraries are built.
 ```
export PSLSE_ROOT=/home/user/pslse
export SNAP_ROOT=/home/user/snap
 ```
 - Run CMake. All dependencies should be resolved automatically: `cmake ..`
 - Run `make` to build all libraries and executables


## How to run
 - Enter an environment where the SNAP action can be attached (i.e. either in simulation or on the POWER machine)
 - Move inside the `build` directory, and `mkdir metadata_store` (this is where the filesystem metadata will reside)
 - Create a mount point (e.g. `mkdir /tmp/metal_fs`)
 - Launch the user-mode file system driver: `./metal_fs -s -f /tmp/metal_fs`
