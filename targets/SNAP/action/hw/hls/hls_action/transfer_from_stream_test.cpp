#include <catch2/catch.hpp>
#include "snap_action_metal.h"
#include "mtl_op_mem.h"

namespace metal {
namespace fpga {

#ifndef NVME_ENABLED
// TEST_CASE("Transfer less than expected") {
//     axi_datamover_command_stream_t s2mm_cmd;
//     axi_datamover_status_ibtt_stream_t s2mm_sts;
//     hls::stream<TransferElement> in, out;

//     TransferElement input, output;

//     input.last = false;
//     input.data.address = 42 * StorageBlockSize;
//     input.data.size = StorageBlockSize;
//     input.data.type = AddressType::Host;
//     in << input;
//     input.last = true;
//     input.data.address = 43 * StorageBlockSize;
//     in << input;

//     axi_datamover_ibtt_status_t status;
//     status.data.num_bytes_transferred() = 8;
//     status.data.end_of_packet() = 8;
//     status.keep = 1;
//     status.last = true;
//     s2mm_sts << status;

//     uint64_t bytes;
//     transfer_from_stream(in, out, s2mm_cmd, s2mm_sts, &bytes);

//     // Input must be read entirely
//     REQUIRE(in.empty());

//     // A datamover command must be issued
//     REQUIRE(s2mm_cmd.size() == 1);
//     s2mm_cmd.read();  // Empty stream to avoid runtime warning

//     // Reported size must equal the datamover result
//     REQUIRE(bytes == 8);

//     // Output must contain two elements
//     REQUIRE(out.size() == 2);
//     output = out.read();
//     output = out.read();

//     // Last output size must be zero
//     REQUIRE(output.data.size == 0);
// }
#endif

}
}
