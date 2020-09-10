Metal FS Prerequisites
======================

These are the current prerequisites for developing Metal FS applications (on an ``amd64`` host):

 - Xilinx Vivado 2018.1

   - Requires specific OS versions, e.g. Ubuntu 16.04
   - As prescribed by the underlying SNAP framework
   - The WebPACK edition is sufficient for simulating operators

 - The following packages, assuming Ubuntu 16.04

   - ``build-essential cmake jq libfuse-dev liblmdb-dev libprotobuf-dev protobuf-compiler``
   - For SNAP: ``git xterm tmux libncurses-dev``
   - GCC 7, must be installed from an unofficial package source
   - A recent version of CMake
   - `node.js and npm <https://github.com/nodesource/distributions/blob/master/README.md#deb>`__ for using third party operator packages

Please also refer to the `sdk-base Dockerfile <https://github.com/osmhpi/metalfs/blob/master/docker/sdk-base/Dockerfile>`__, which should always reference the most up-to date information regarding prerequisites.
