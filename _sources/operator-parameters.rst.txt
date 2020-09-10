Operator Parameters
===================

Typically, an operator needs additional data at runtime to perform its data transformation.
Such parameters apply to the entire data stream and could be encryption keys, numeric values for data filters or enumeration values to switch between different modes of operation.

Therefore, Metal FS provides capabilities to define parameters as part of the operator hardware description and handles filling in the parameter data at runtime.

Receiving Operator Parameters in Hardware
*****************************************

In this section, the use of operator parameters is discussed in the context of HLS operators.
If you're using another hardware description language, you have to manually add the logic to receive parameter data through the AXI register interface of the operator, at an address offset of your choice.

With HLS, parameters are added as part of your top-level method signature.
Here's an example:

.. code-block:: cpp

    void filter(
            mtl_stream &in,
            mtl_stream &out,
            uint64_t lower_bound,
            uint64_t upper_bound
        ) {
        // The basic operator directives:
        #pragma HLS INTERFACE axis port=in name=axis_input
        #pragma HLS INTERFACE axis port=out name=axis_output
        #pragma HLS INTERFACE s_axilite port=return bundle=control

        // Parameter-specific directives
        #pragma HLS INTERFACE s_axilite port=lower_bound bundle=control offset=0x100
        #pragma HLS INTERFACE s_axilite port=upper_bound bundle=control offset=0x110

        // [Operator logic]
    }


Here's another example with a parameter that contains a buffer of bytes.
Note that this operator also uses the optional ``prepare`` phase, which allows it to perform potentially time-consuming initialization steps before the actual processing phase.

.. code-block:: cpp

    void encrypt(
            mtl_stream &in,
            mtl_stream &out,
            ap_uint<8> prepare,
            ap_uint<8> key_bytes[16]
        ) {
        // Basic operator directives
        #pragma HLS INTERFACE axis port=in name=axis_input
        #pragma HLS INTERFACE axis port=out name=axis_output
        #pragma HLS INTERFACE s_axilite port=return bundle=control

        // The 'prepare' parameter *must* be located at this offset
        #pragma HLS INTERFACE s_axilite port=prepare bundle=control offset=0x010

        // Additional parameters
        #pragma HLS INTERFACE s_axilite port=key_bytes bundle=control offset=0x100

        if (prepare) {
            // Perform initialization of internal state here.
            // Do *not* read or write to the in/out streams here.
        } else {
            // The regular operator processing loop goes here.
        }
    }


The specific offsets of custom parameters can be freely chosen, as long as they don't interfere with the HLS register interface and the `prepare` parameter.
A starting offset of ``0x100`` works well.

Declaring Parameters in the Operator Manifest
*********************************************

The detailed schema of the operator manifest is described [here](operator_manifest).
For instance, the manifest of the ``encrypt`` operator above could look like this:

.. code-block:: json

    {
        "main": "encrypt",
        "description": "Encrypt the data",
        "prepare_required": true,
        "options": {
            "key": {
                "short": "k",
                "type": { "type": "buffer", "size": 16 },
                "description": "The encryption key to use",
                "offset": 256
            }
        }
    }


Note that the parameter offset is given as a decimal value (0x100 = 256).

Setting Operator Parameters (C++ API)
*************************************

Once you've obtained an :cpp:class:`~metal::Operator` object from the :cpp:class:`~metal::OperatorFactory`, refer to the desired parameter by name to provide its value.
Example:

.. code-block:: cpp

    auto keyBytes = std::make_shared<std::vector<char>>(16);

    SnapAction fpga;
    auto factory = OperatorFactory::fromFPGA(fpga);

    auto encrypt = factory.createOperator("encrypt");
    encrypt.setOption("key", keyBytes);
