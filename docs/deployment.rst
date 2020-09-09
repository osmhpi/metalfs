Deployment Guide
================

Once the ``make image`` process has finished, you will find the FPGA bitstream in ``build/<target>/snap/hardware/build/Images/<bitstream>.bin``. Copy this file to your POWER machine.

Deploy it to your FPGA using the flash script from `capi-utils <https://github.com/ibm-capi/capi-utils>`__, e.g.: ``capi-flash-script <bitstream>.bin``.

Afterwards, perform the initial action discovery using `SNAP <https://github.com/open-power/snap>`__ tooling:

.. code-block:: bash

    git clone https://github.com/open-power/snap && cd snap
    make software
    # As root:
    ./software/tools/snap_maint -v
    # If you're using NVMe storage on the FPGA card
    ./software/tools/snap_nvme_init


Now you can run either your own software that uses Metal FS as a library or start the Metal FS file system driver.
To perform the latter in a container, use

.. code-block:: bash

    docker run -d \
        --name metalfs \
        --device /dev/cxl/afu0.0s \
        --device /dev/fuse \
        --security-opt apparmor=unconfined \
        --cap-add SYS_ADMIN \
        metalfs/metalfs:latest \
            metal-driver -s -f /mnt --metadata ./metadata

Note that the file system metadata is not persisted outside of the container in this configuration.

Now, you could start a shell in the container to access the file system in `/mnt`:

.. code-block:: bash

    docker exec -it metalfs bash

