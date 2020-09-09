Image Manifest
==============

The image manifest (``image.json``) contains configuration options for the FPGA image.

Example:

.. code-block:: json

    {
        "target": "SNAP/N250S_NVMe",
        "streamBytes": 32,
        "operators": {
            "colorfilter": "./colorfilter",
            "encrypt": "@metalfs/encrypt"
        }
    }

Detailed Property Descriptions
******************************

**target**:
One of the supported hardware targets for Metal FS, including the hardware shell to use (for now, only ``SNAP`` is supported).
The available targets are defined `here <https://github.com/osmhpi/metal_fs/tree/master/targets>`__.

**streamBytes**:
Defines the global stream width for all operators in the image.
Note that not all operators support different stream widths.
Recommended values: 8, 16, 32, 64

**operators**:
A JSON object, where the keys (string) define the unique operator identifier in the final image and the values (string) provide
a path to the operator sources.
If the path starts with ``./``, a local operator is assumed, otherwise it is interpreted as a npm package name.
Note that npm packages are not downloaded automatically during the build process; you have to run ``npm install`` manually before.
