# Metal FS

Metal FS augments the Linux file system to expose FPGA-based near-storage accelerators.

Metal FS was presented at the OpenPOWER Summit Europe 2018: [[Recording]](https://www.youtube.com/watch?v=tkBXc47u8eE) [[Slides]](https://openpowerfoundation.org/wp-content/uploads/2018/10/Max-Plauth.OPSE_HPI_MetalFS-2.pdf)

## Metal FS Operators

Operators are the fundamental building blocks of any Metal FS application. They can be programmed in Vivado HLS C, without any knowledge about hardware description languages needed:
```cpp
void op_example(mtl_stream &in, mtl_stream &out) {
    mtl_stream_element element;
    do {
        element = in.read();
        // insert operator logic here
        out.write(element);
    } while (!element.last);
}
```
More examples and builtin operators can be found [here](src/metal_fpga/hw/hls/hls_action).

## How to build

Metal FS depends on IBM CAPI SNAP and thus works with POWER8 and POWER9 servers that are equipped with CAPI-enabled FPGA cards.
Development and simulation requires Xilinx Vivado on an x86_64-based computer.

### Hardware (Development)
 - Set up a working [CAPI SNAP](https://github.com/open-power/capi) build environment
 - Configure SNAP to use an external HDL action, rooted in `metal_fs/src/metal_fpga`
   - Enable SDRAM and NVMe if your FPGA supports it. Operator Pipelines can still be used if not.
 - Run `make model` or `make image` for building a simulation model or an FPGA bitstream, respectively

### Software (Development + Runtime)

 - CMake is required to build the software, as well as FUSE (`libfuse-dev` on Ubuntu), LMDB (`liblmdb-dev`) and libcxl ( `libcxl-dev` or part of [PSLSE](https://github.com/ibm-capi/pslse) when simulating)
 - Create a build directory: `mkdir build && cd build`
 - Specify environment variables pointing to SNAP (and if necessary PSLSE). Make sure that the libcxl.a and libsnap.a libraries are built.
 ```
export PSLSE_ROOT=/home/user/pslse
export SNAP_ROOT=/home/user/snap
 ```
 - Run CMake. All dependencies should be resolved automatically: `cmake ..`
 - Run `make` to build all libraries and executables

Using the [Dockerfile](Dockerfile), you can also build a Metal FS Docker image to run the file system driver as a container on POWER.

## How to run

 - Enter an environment where the SNAP action can be attached (i.e. either in SNAP's `make sim` shell or on the POWER machine)
 - Create a mount point (e.g. `mkdir /tmp/metal_fs`)
 - Launch the user-mode file system driver: `build/metal_fs -s -f /tmp/metal_fs`
