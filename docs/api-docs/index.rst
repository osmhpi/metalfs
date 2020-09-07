.. toctree::
   :maxdepth: 2
   :caption: Contents:
   :hidden:

   prerequisites
   tutorial
   operator-parameters
   docker-dev
   buildpacks
   deployment
   image-manifest
   operator-manifest
   metal-pipeline
   metal-filesystem
   metal-filesystem-pipeline
   genindex


Metal FS
========


.. image:: assets/metalfs.png
  :alt: Metal FS
  :width: 200px

Metal FS is a near-storage compute framework that combines FPGA compute kernels ('operators') with a file system.
It is open-source on `GitHub <https://github.com/osmhpi/metalfs>`__.

It currently supports CAPI-enabled FPGA cards in IBM POWER systems.

Development and simulation of operators is possible using only free tools (x86_64).

Getting started
***************

The :ref:`tutorial <Tutorial>` will cover how to create a Metal FS FPGA image that consists of pre-defined operators as well as a custom image transformation kernel.

For operator development you can either use a :ref:`dockerized development environment <Docker-based Development Environment>` (recommended), or set up the :ref:`prerequisites <Metal FS Prerequisites>` manually.


Running on POWER
****************

If you have access to a POWER8/POWER9 system equipped with a suitable FPGA, please follow the :ref:`deployment guide <Deployment Guide>`.

Advanced Topics
***************

Refer to these guides for advanced Metal FS use cases:

- :ref:`Operator Parameters`
- :ref:`Image Manifest Documentation <Image Manifest>`
- :ref:`Operator Manifest Documentation <Operator Manifest>`
- Operator Pipeline API (C++)

  - Basic usage
  - Filesystem Data Source / Data Sink

- Inspecting Simulation Waveforms
- Operator Profiling at Runtime
- Operator Hardware Interface
- Benchmarking Data Generator
- :ref:`Operator Buildpacks <Buildpacks>`
- FPGA Targets

Feedback
********

We welcome your feedback to Metal FS!

If you have questions or remarks, please create an `issue <https://github.com/osmhpi/metalfs/issues>`__ on the main `GitHub <https://github.com/osmhpi/metalfs>`__ repository.
