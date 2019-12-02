#include <catch2/catch.hpp>
#include "snap_action_metal.h"
#include "mtl_op_mem.h"

namespace metal {
namespace fpga {

#ifndef NVME_ENABLED
TEST_CASE("Issue partial transfer") {
    hls::stream<TransferElement> partial_transfers;

    Address xfer;
    xfer.addr = 42 * StorageBlockSize;
    xfer.type = AddressType::Host;
    xfer.map = MapType::None;
    xfer.size = StorageBlockSize;

    issue_partial_transfers(xfer, dram_read_extmap, partial_transfers);

    REQUIRE(partial_transfers.size() == 1);

    TransferElement result;
    partial_transfers >> result;

    REQUIRE(result.last);
    REQUIRE(result.data.address == xfer.addr);
    REQUIRE(result.data.type == xfer.type);
    REQUIRE(result.data.size == xfer.size);
}

TEST_CASE("Load NVMe blocks does nothing") {
    hls::stream<TransferElement> in, out;
    TransferElement input, output;

    input.last = true;
    input.data.address = 42 * StorageBlockSize;
    input.data.size = StorageBlockSize;
    input.data.type = AddressType::Host;
    in << input;

    load_nvme_data(in, out);

    REQUIRE(out.size() == 1);

    out >> output;
    REQUIRE(output.last);
    REQUIRE(output.data.address == input.data.address);
    REQUIRE(output.data.size == input.data.size);
    REQUIRE(output.data.type == input.data.type);
}

TEST_CASE("Transfer to stream returns") {
    axi_datamover_command_stream_t mm2s_cmd;
    axi_datamover_status_stream_t mm2s_sts;
    hls::stream<TransferElement> in;

    TransferElement input;

    input.last = true;
    input.data.address = 42 * StorageBlockSize;
    input.data.size = StorageBlockSize;
    input.data.type = AddressType::Host;
    in << input;

    axi_datamover_status_t status;
    status.data = StorageBlockSize;
    status.keep = 1;
    status.last = true;
    mm2s_sts << status;

    transfer_to_stream(in, mm2s_cmd, mm2s_sts);

    REQUIRE(mm2s_cmd.size() == 1);
    mm2s_cmd.read();  // Empty stream to avoid runtime warning
}

TEST_CASE("Read memory from host") {
    axi_datamover_command_stream_t mm2s_cmd;
    axi_datamover_status_stream_t mm2s_sts;
    snapu32_t *random_ctrl = nullptr;

    Address config;
    config.addr = 42 * StorageBlockSize;
    config.type = AddressType::Host;
    config.map = MapType::None;
    config.size = StorageBlockSize;

    axi_datamover_status_t status;
    status.data = StorageBlockSize;
    status.keep = 1;
    status.last = true;
    mm2s_sts << status;

    op_mem_read(mm2s_cmd, mm2s_sts, random_ctrl, config);

    REQUIRE(mm2s_cmd.size() == 1);
    mm2s_cmd.read();  // Empty stream to avoid runtime warning
}
#endif

#ifdef NVME_ENABLED
TEST_CASE("Load NVMe blocks does something") {
    hls::stream<TransferElement> in, out;
    hls::stream<uint64_t> nvme_transfers;
    NVMeCommandStream nvme_cmd; NVMeResponseStream nvme_resp;
    TransferElement input, output;

    input.last = true;
    input.data.address = 42 * StorageBlockSize;
    input.data.size = StorageBlockSize;
    input.data.type = AddressType::NVMe;
    in << input;

    nvme_transfers << 0;

    nvme_resp << 1;

    load_nvme_data(in, out, nvme_transfers, nvme_cmd, nvme_resp);

    REQUIRE(nvme_cmd.size() == 1);
    nvme_cmd.read();  // Empty stream to avoid runtime warning
    REQUIRE(out.size() == 1);

    out >> output;
    REQUIRE(output.last);
    REQUIRE(output.data.address == input.data.address);
    REQUIRE(output.data.size == input.data.size);
    REQUIRE(output.data.type == input.data.type);
}
#endif

}
}
