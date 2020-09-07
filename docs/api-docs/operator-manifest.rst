Operator Manifest
=================

The operator manifest (``operator.json``) specifies operator metadata.

Example:

.. code-block:: json

    {
        "main": "changecase",
        "description":
            "Transform ASCII strings to lower- or uppercase",
        "prepare_required": false,
        "options": {
            "lowercase": {
                "short": "l",
                "type": "bool",
                "description": "Transform to lowercase",
                "offset": 256
            }
        },
        "devImage": {
            "streamBytes": 8
        }
    }

Detailed Property Descriptions
******************************

**main**:
The entrypoint/top-level component of the operator.
Depending on the [buildpack](buildpacks) (e.g. HLS or HDL), this is used during the IP build process.

**description**:
Usage information about the operator. Displayed as part of the help text provided by the operator executable.
Example:

.. code-block:: bash

    /metal_fs $ ./operators/colorfilter --help
    Convert bitmap to grayscale

**prepare_required**:
Whether the operator requires to prepare its internal state before processing starts.
*Optional* (default: ``false``).

This is typically used to avoid costly parameter ingestion computations on each operator invocation.
Instead, the processing phase is only trigged when the parameter values change, i.e. once per :cpp:class:`~metal::SnapPipelineRunner` invocation. See also: :ref:`Operator Parameters`.

**options**:
JSON object specifying configuration options for the operator to be provided at runtime.
The keys specify the long option names which are used for passing parameters to the operator executables or the operator objects in the C++ API.
The values are another JSON object with the following properties:

 - **short**: Short parameter name (for passing arguments to the operator executable). *Optional.*
 - **type**: One of the supported parameter types:

   - ``"bool"``
   - ``"int"``
   - ``{ "type": "buffer", "size": <number> }`` (``size`` can be chosen arbitrarily)

 - **description**: Description shown in help text of the operator executable
 - **offset**: Offset of the parameter in the operator register space

**devImage**:
When creating simulation models during operator development, a simple Metal FS image containing the operator is generated
in the background.
If the operator does not support arbitrary stream widths, the development image can be configured to use the stream width specified here.
