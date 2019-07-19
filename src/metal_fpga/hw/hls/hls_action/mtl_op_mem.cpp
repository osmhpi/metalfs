#include "mtl_op_mem.h"
#include "mtl_endian.h"
#include "snap_action_metal.h"
#include "axi_switch.h"

#include <snap_types.h>

namespace metal {
namespace fpga {

Address read_mem_config;
Address write_mem_config;

mtl_extmap_t nvme_read_extmap;
mtl_extmap_t nvme_write_extmap;

mtl_retc_t op_mem_set_config(Address &address, snap_bool_t read, Address &config, snapu32_t *data_preselect_switch_ctrl) {
    config = address;

    // Configure Data Preselector

    if (read) {
        switch (config.type) {
            case AddressType::Host:
            case AddressType::CardDRAM:
            case AddressType::NVMe: {
                switch_set_mapping(data_preselect_switch_ctrl, 0, 0); // DataMover -> Metal Switch
                break;
            }
            case AddressType::Random: {
                switch_set_mapping(data_preselect_switch_ctrl, 1, 0); // Stream Gen -> Metal Switch
                break;
            }
            default: break;
        }
    } else {
        switch (config.type) {
            case AddressType::Host:
            case AddressType::CardDRAM:
            case AddressType::NVMe: {
                switch_set_mapping(data_preselect_switch_ctrl, 2, 1); // Metal Switch -> DataMover
                switch_disable_output(data_preselect_switch_ctrl, 2); // X -> Stream Sink
                break;
            }
            case AddressType::Null: {
                switch_disable_output(data_preselect_switch_ctrl, 1); // X -> DataMover
                switch_set_mapping(data_preselect_switch_ctrl, 2, 2); // Metal Switch -> Stream Sink
                break;
            }
            default: break;
        }
    }

    switch_commit(data_preselect_switch_ctrl);

    return SNAP_RETC_SUCCESS;
}

const uint64_t DRAMBaseOffset = 0x8000000000000000;

const uint64_t NVMeDRAMReadOffset = DRAMBaseOffset + 0;
const uint64_t NVMeDRAMWriteOffset = DRAMBaseOffset + (1u << 31); // 2 GiB

template<bool Write>
uint64_t resolve_effective_address (Address &config, uint64_t current_offset) {
    switch (config.type) {
        case AddressType::NVMe:
            // Support transfer lengths of > 2GiB, but wrap around
            return ((Write ? NVMeDRAMWriteOffset : NVMeDRAMReadOffset) + (current_offset % (1u << 31)));
        case AddressType::CardDRAM:
            return DRAMBaseOffset + config.addr + current_offset;
        //     // If we support block mappings into DRAM as well, perform translation here.
        //     return ...
        default:
            break;
    }

    return config.addr + current_offset;
}

template<typename TReadStreamElement>
TReadStreamElement read(hls::stream<TReadStreamElement> &stream, hls::stream<ap_uint<1>> &signal) {
    signal.read();
    return stream.read();
}

template<typename TWriteStreamElement>
void write(hls::stream<TWriteStreamElement> &stream, TWriteStreamElement &data, hls::stream<ap_uint<1>> &signal) {
    stream.write(data);
    signal.write(0);
}

template<typename TReadStreamElement, typename TWriteStreamElement>
TReadStreamElement writeThenRead(hls::stream<TWriteStreamElement> &writeStream, TWriteStreamElement &data, hls::stream<TReadStreamElement> &readStream) {

    // HLS is not aware of a dependency between the write and read streams, so it seems to synthesize logic
    // that attempts to read before writing. In order to clarify the dependency we have to get a little
    // creative here...

    hls::stream<ap_uint<1>> signal;

#pragma HLS DATAFLOW
    write(writeStream, data, signal);
    return read(readStream, signal);
}

template<bool Write>
void issue_pmem_transfer_command(uint64_t current_offset, mtl_extmap_t &map, NVMeCommandStream &nvme_cmd, NVMeResponseStream &nvme_resp) {
    mtl_extmap_seek(map, current_offset / StorageBlockSize);

    auto logical_block_offset = mtl_extmap_pblock(map);
    auto drive_id = logical_block_offset[0];
    auto physical_block_offset = (logical_block_offset >> 1) * (StorageBlockSize / 512);

    NVMeCommand cmd;
    cmd.dram_offset() = (Write ? NVMeDRAMWriteOffset : NVMeDRAMReadOffset) + current_offset;
    cmd.nvme_block_offset() = physical_block_offset;
    cmd.num_blocks() = StorageBlockSize / 512;  // 512 = native block size
    cmd.drive() = drive_id;

    writeThenRead(nvme_cmd, cmd, nvme_resp);
}

template<typename TStatusWord>
TStatusWord issue_transfer_command(uint64_t bytes_remaining, uint64_t effective_address, axi_datamover_command_stream_t &dm_cmd, hls::stream<TStatusWord> &dm_sts) {
    snap_bool_t end_of_frame = bytes_remaining <= StorageBlockSize;
    ap_uint<23> transfer_bytes = end_of_frame ? bytes_remaining : StorageBlockSize;

    axi_datamover_command_t cmd = 0;
    const int N = 64; // Address width

    cmd((N+31), 32) = effective_address;
    cmd[30]         = end_of_frame;
    cmd[23]         = 1;                    // AXI burst type: INCR
    cmd(22, 0)      = transfer_bytes;

    return writeThenRead(dm_cmd, cmd, dm_sts);
}

void op_mem_read(
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    snapu32_t *random_ctrl,
#ifdef NVME_ENABLED
    NVMeCommandStream &nvme_read_cmd,
    NVMeResponseStream &nvme_read_resp,
#endif
    Address &config) {

    switch (config.type) {
        case AddressType::Host:
        case AddressType::CardDRAM:
        case AddressType::NVMe: {
            uint64_t bytes_read = 0;
            while (bytes_read < config.size) {
                uint64_t effective_address = resolve_effective_address</*write=*/false>(config, bytes_read);

            #ifdef NVME_ENABLED
                if (config.type == AddressType::NVMe)
                    issue_pmem_transfer_command</*write=*/false>(bytes_read, nvme_read_extmap, nvme_read_cmd, nvme_read_resp);
            #endif

                // Stream data in block-sized chunks (64K)
                uint64_t bytes_remaining = config.size - bytes_read;
                issue_transfer_command(bytes_remaining, effective_address, mm2s_cmd, mm2s_sts);
                bytes_read += bytes_remaining <= StorageBlockSize ? bytes_remaining : StorageBlockSize;
            }
            break;
        }
        case AddressType::Random: {
            random_ctrl[0x10 / sizeof(snapu32_t)] = config.size;
            random_ctrl[0] = 1;  // ap_start = true (self-clearing)
            break;
        }
        default: break;
    }
}

ap_uint<32> read_status(axi_datamover_status_ibtt_stream_t &s2mm_sts) {
    axi_datamover_ibtt_status_t status = s2mm_sts.read();
    return status.data;
}

uint64_t op_mem_write(
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
#ifdef NVME_ENABLED
    NVMeCommandStream &nvme_write_cmd,
    NVMeResponseStream &nvme_write_resp,
#endif
    Address &config) {

    switch (config.type) {
        case AddressType::Host:
        case AddressType::CardDRAM:
        case AddressType::NVMe: {
            uint64_t bytes_written = 0;

            for (;;) {
                uint64_t bytes_remaining = config.size - bytes_written;
                uint64_t effective_address = resolve_effective_address</*write=*/true>(config, bytes_written);

                auto result = issue_transfer_command(bytes_remaining, effective_address, s2mm_cmd, s2mm_sts);
                auto done = result.data[31] /* = end of packet */ || bytes_remaining <= StorageBlockSize;
                auto num_bytes_transferred = result.data(30, 8);

            #ifdef NVME_ENABLED
                if (config.type == AddressType::NVMe)
                    issue_pmem_transfer_command</*write=*/true>(bytes_written, nvme_write_extmap, nvme_write_cmd, nvme_write_resp);
            #endif

                bytes_written += num_bytes_transferred;

                if (done) {
                    return bytes_written;
                }
            }
            break;
        }
        default: break;
    }

    return 0;
}

}  // namespace fpga
}  // namespace metal
