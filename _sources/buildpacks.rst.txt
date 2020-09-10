Buildpacks
==========

Metal FS provides build packs for creating operators from different types of hardware descriptions as well as to create FPGA images from an image manifest.
Build packs are ``make`` based.

Image Build Pack
****************

Add the following line in a Makefile that sits next to your ``image.json``:

.. code-block:: Makefile

    include $(METAL_ROOT)/buildpacks/image/image.mk


You will now have access to the following ``make`` targets (resembling SNAP ``make`` targets):

 - ``make help`` -- shows help
 - ``make model`` -- builds a simulation model
 - ``make sim`` -- starts simulation and the Metal FS filesystem driver (under `/mnt`)
 - ``make image`` -- builds the FPGA bitstream

Operator Buildpack (HLS and HDL)
********************************

Add this to a Makefile that sits next to your ``operator.json``:

HLS:

.. code-block:: Makefile

    srcs += changecase.cpp
    include $(METAL_ROOT)/buildpacks/hls/hls.mk

HDL:

.. code-block:: Makefile

    srcs += changecase.vhd
    include $(METAL_ROOT)/buildpacks/hdl/hdl.mk

The buildpack offers the following targets, so you can quickly build a simulation model from your operator:

 - ``make help`` -- shows help
 - ``make devmodel`` -- (similar to ``make model`` from the image buildpack)
 - ``make sim`` -- starts simulation with the devmodel

For setting the stream width to be used in the devmodel image, please refer to the [operator manifest](operator_manifest) docs.

The HLS buildpack additionally offers the following target:

 - ``make test`` -- runs the HLS testbench
