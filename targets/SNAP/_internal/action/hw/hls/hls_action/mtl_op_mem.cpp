#include "ap_utils.h"
#include "mtl_op_mem.h"
#include "mtl_endian.h"
#include "snap_action_metal.h"
#include "axi_switch.h"

#include <snap_types.h>

namespace metal {
namespace fpga {

Address read_mem_config;
Address write_mem_config;

mtl_extmap_t dram_read_extmap;
mtl_extmap_t dram_write_extmap;
#ifdef NVME_ENABLED
mtl_extmap_t nvme_read_extmap;
mtl_extmap_t nvme_write_extmap;
#endif

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

const uint64_t NVMeDRAMBaseOffset = 0x200000000;  // According to https://github.com/open-power/snap/blob/master/hardware/doc/NVMe.md
const uint64_t NVMeDRAMReadOffset  = 0;
const uint64_t NVMeDRAMWriteOffset = (1u << 31); // 2 GiB

void issue_block_transfer_command(uint64_t transfer_bytes, snap_bool_t end_of_frame, uint64_t effective_address, axi_datamover_command_stream_t &dm_cmd) {
    DatamoverCommand cmd;
    cmd.effective_address() = effective_address;
    cmd.end_of_frame()      = end_of_frame;
    cmd.axi_burst_type()    = 1; // INCR
    cmd.transfer_bytes()    = transfer_bytes;

    dm_cmd.write(cmd);
}

#ifdef NVME_ENABLED
void issue_nvme_block_transfer_command(uint64_t nvme_address, uint64_t dram_address, NVMeCommandStream &nvme_cmd) {
    // Both NVMe address and DRAM address may be unaligned, but for NVMe transfers we don't care
    snapu64_t logical_block_offset = nvme_address / StorageBlockSize;
    snapu64_t physical_block_offset = logical_block_offset * (StorageBlockSize / 512);
    snapu64_t aligned_dram_address = dram_address;
    aligned_dram_address(StorageBlockSizeD-1, 0) = 0;

    NVMeCommand cmd;
    cmd.dram_offset()       = aligned_dram_address;
    cmd.nvme_block_offset() = physical_block_offset;
    cmd.num_blocks()        = (StorageBlockSize / 512) - 1;  // 512 = native block size, zero-based
    cmd.drive()             = 0;

    nvme_cmd.write(cmd);
}

void issue_nvme_block_transfer_command_sync(uint64_t nvme_address, uint64_t dram_address, NVMeCommandStream &nvme_cmd, NVMeResponseStream &nvme_resp) {
    issue_nvme_block_transfer_command(nvme_address, dram_address, nvme_cmd);
    ap_wait();  // Tick-tock
    nvme_resp.read();
}
void preload_nvme_blocks(const Address &address, mtl_extmap_t &dram_extentmap, mtl_extmap_t &nvme_extentmap, NVMeCommandStream &nvme_read_cmd, NVMeResponseStream &nvme_read_resp) {
    snapu64_t start_addr = address.addr;
    snapu64_t end_addr = start_addr + address.size;
    snapu64_t last_byte_addr = start_addr + (address.size > 0 ? address.size : 1) - 1;

    auto startAddressIsOdd = start_addr(StorageBlockSizeD-1, 0) != 0;
    auto endAddressIsOdd = end_addr(StorageBlockSizeD-1, 0) != 0;
    auto endBlockIsDifferentFromStartBlock = start_addr(63, StorageBlockSizeD) != last_byte_addr(63, StorageBlockSizeD);

    if (startAddressIsOdd) {
        uint64_t dram_address = 0;
        switch (address.map) {
        case MapType::NVMe: {
            dram_address = NVMeDRAMWriteOffset + (start_addr % (1u << 31)); // Where to put data in DRAM
            break;
        }
        case MapType::DRAMAndNVMe: {
            uint64_t pagefile_offset = start_addr % PagefileSize;
            mtl_extmap_seek(dram_extentmap, pagefile_offset / StorageBlockSize);
            dram_address = mtl_extmap_pblock(dram_extentmap) * StorageBlockSize; // Where to put data in DRAM
            break;
        }
        default: break;
        }

        mtl_extmap_seek(nvme_extentmap, start_addr / StorageBlockSize);
        uint64_t nvme_address = mtl_extmap_pblock(nvme_extentmap) * StorageBlockSize; // Where to read from NVMe

        issue_nvme_block_transfer_command_sync(nvme_address, dram_address, nvme_read_cmd, nvme_read_resp);
    }

    if (endAddressIsOdd && (endBlockIsDifferentFromStartBlock || !startAddressIsOdd)) {
        uint64_t dram_address = 0;
        switch (address.map) {
        case MapType::NVMe: {
            dram_address = NVMeDRAMWriteOffset + (end_addr % (1u << 31)); // Where to put data in DRAM
            break;
        }
        case MapType::DRAMAndNVMe: {
            uint64_t pagefile_offset = end_addr % PagefileSize;
            mtl_extmap_seek(dram_extentmap, pagefile_offset / StorageBlockSize);
            dram_address = mtl_extmap_pblock(dram_extentmap) * StorageBlockSize; // Where to put data in DRAM
            break;
        }
        default: break;
        }

        mtl_extmap_seek(nvme_extentmap, end_addr / StorageBlockSize);
        uint64_t nvme_address = mtl_extmap_pblock(nvme_extentmap) * StorageBlockSize; // Where to read from NVMe

        issue_nvme_block_transfer_command_sync(nvme_address, dram_address, nvme_read_cmd, nvme_read_resp);
    }
}
#endif

enum class TransferType {
    Read,
    Write
};

template<TransferType T>
void issue_partial_transfers(const Address& transfer
    , mtl_extmap_t &dram_extentmap
    , hls::stream<TransferElement> &partial_transfers
#ifdef NVME_ENABLED
    , mtl_extmap_t &nvme_extentmap
    , hls::stream<uint64_t> &nvme_transfers
#endif
) {
    uint64_t bytes_transferred = 0;

    // Perform seek in the extent maps
    switch (transfer.map) {
        case MapType::DRAM:
            mtl_extmap_seek(dram_extentmap, transfer.addr / StorageBlockSize);
            break;
#ifdef NVME_ENABLED
        case MapType::NVMe:
            mtl_extmap_seek(nvme_extentmap, transfer.addr / StorageBlockSize);
            break;
        case MapType::DRAMAndNVMe:
            mtl_extmap_seek(nvme_extentmap, transfer.addr / StorageBlockSize);
            mtl_extmap_seek(dram_extentmap, (transfer.addr % PagefileSize) / StorageBlockSize);
            break;
#endif
        default:
            break;
    }

    while (bytes_transferred < transfer.size) {
        #pragma HLS pipeline II=64
        uint64_t bytes_remaining = transfer.size - bytes_transferred;
        uint64_t current_address = transfer.addr + bytes_transferred;
        uint64_t intra_block_offset = current_address % StorageBlockSize;

        // Always perform block-aligned transfers (i.e. a transfer does not cross a block boundary)
        auto transfer_limit = StorageBlockSize - intra_block_offset;
        auto bytes_this_round = bytes_remaining <= transfer_limit ? bytes_remaining : transfer_limit;

        // Perform address translation
        TransferElement partial_transfer = {};
        switch (transfer.map) {
            case MapType::DRAM: {
                partial_transfer.data.address = (mtl_extmap_pblock(dram_extentmap) * StorageBlockSize) + intra_block_offset;
                mtl_extmap_next(dram_extentmap);
                break;
            }
#ifdef NVME_ENABLED
            case MapType::NVMe: {
                partial_transfer.data.address = (T == TransferType::Read ? NVMeDRAMReadOffset : NVMeDRAMWriteOffset) + (current_address % (1u << 31)); // Where to put data in DRAM
                nvme_transfers << (mtl_extmap_pblock(nvme_extentmap) * StorageBlockSize) + intra_block_offset; // Where to read from NVMe
                mtl_extmap_next(nvme_extentmap);
                break;
            }
            case MapType::DRAMAndNVMe:{
                partial_transfer.data.address = (mtl_extmap_pblock(dram_extentmap) * StorageBlockSize) + intra_block_offset; // Where to put data in DRAM
                nvme_transfers << (mtl_extmap_pblock(nvme_extentmap) * StorageBlockSize) + intra_block_offset; // Where to read from NVMe
                mtl_extmap_next(dram_extentmap);
                mtl_extmap_next(nvme_extentmap);
                break;
            }
#endif
            default: {
                partial_transfer.data.address = current_address;
                break;
            }
        }

        partial_transfer.data.size = bytes_this_round;
        partial_transfer.data.type = transfer.type;
        partial_transfer.last = bytes_this_round == bytes_remaining;

        partial_transfers << partial_transfer;

        bytes_transferred += bytes_this_round;
    }
}

void load_nvme_data(hls::stream<TransferElement> &in, hls::stream<TransferElement> &out
#ifdef NVME_ENABLED
    , hls::stream<uint64_t> &nvme_transfers
    , NVMeCommandStream &nvme_cmd
    , NVMeResponseStream &nvme_resp
#endif
) {

#ifdef NVME_ENABLED
    hls::stream<TransferElement> pendingTransfers;
    #pragma HLS stream variable=pendingTransfers depth=16
    #pragma HLS dataflow
    {
        TransferElement currentTransfer;
        do {
            #pragma HLS pipeline
            in >> currentTransfer;

            if (currentTransfer.data.type != AddressType::NVMe) {
                pendingTransfers << currentTransfer;
                continue;
            }

            auto dram_address = NVMeDRAMBaseOffset + currentTransfer.data.address;

            const uint64_t nvme_address = nvme_transfers.read();  // There must be an item in the queue
            issue_nvme_block_transfer_command(nvme_address, dram_address, nvme_cmd);

            pendingTransfers << currentTransfer;
        } while (!currentTransfer.last);
    }
    {
        TransferElement currentTransfer;
        do {
            #pragma HLS pipeline
            pendingTransfers >> currentTransfer;
            if (currentTransfer.data.type == AddressType::NVMe) {
                nvme_resp.read();
            }
            out << currentTransfer;
        } while (!currentTransfer.last);
    }
#else
    // Passthrough (no NVMe)
    TransferElement currentTransfer;
    do {
        #pragma HLS pipeline
        in >> currentTransfer;
        out << currentTransfer;
    } while (!currentTransfer.last);
#endif
}

void transfer_to_stream(hls::stream<TransferElement> &in, axi_datamover_command_stream_t &dm_cmd, axi_datamover_status_stream_t &dm_sts) {
    hls::stream<TransferElement> pendingTransfers;

    #pragma HLS dataflow
    {
        TransferElement currentTransfer;
        do {
            #pragma HLS pipeline
            in >> currentTransfer;
            uint64_t address = currentTransfer.data.address;
            if (currentTransfer.data.type == AddressType::CardDRAM || currentTransfer.data.type == AddressType::NVMe) {
                address += DRAMBaseOffset;
            }

            issue_block_transfer_command(currentTransfer.data.size, currentTransfer.last, address, dm_cmd);

            pendingTransfers << currentTransfer;
        } while (!currentTransfer.last);
    }
    {
        TransferElement currentTransfer;
        do {
            #pragma HLS pipeline
            pendingTransfers >> currentTransfer;
            dm_sts.read();
        } while (!currentTransfer.last);
    }
}

void write_nvme_data(hls::stream<TransferElement> &in
#ifdef NVME_ENABLED
    , hls::stream<uint64_t> &nvme_transfers
    , NVMeCommandStream &nvme_cmd
    , NVMeResponseStream &nvme_resp
#endif
) {

#ifdef NVME_ENABLED
    hls::stream<TransferElement> pendingTransfers;
    #pragma HLS stream variable=pendingTransfers depth=16
    #pragma HLS dataflow
    {
        TransferElement currentTransfer;
        do {
            #pragma HLS pipeline
            in >> currentTransfer;

            if (currentTransfer.data.type != AddressType::NVMe) {
                pendingTransfers << currentTransfer;
                continue;
            }

            auto dram_address = NVMeDRAMBaseOffset + currentTransfer.data.address;

            const uint64_t nvme_address = nvme_transfers.read();  // There must be an item in the queue

            if (currentTransfer.data.size > 0) {  // We need transfers of size==0 to empty the nvme_transfers queue
                issue_nvme_block_transfer_command(nvme_address, dram_address, nvme_cmd);
            }

            pendingTransfers << currentTransfer;
        } while (!currentTransfer.last);
    }
    {
        TransferElement currentTransfer;
        do {
            #pragma HLS pipeline
            pendingTransfers >> currentTransfer;
            if (currentTransfer.data.type == AddressType::NVMe) {
                nvme_resp.read();
            }
        } while (!currentTransfer.last);
    }
#else
    // Discard (no NVMe)
    TransferElement currentTransfer;
    do {
        in >> currentTransfer;
    } while (!currentTransfer.last);
#endif
}


void transfer_from_stream(hls::stream<TransferElement> &in, hls::stream<TransferElement> &out, axi_datamover_command_stream_t &dm_cmd, axi_datamover_status_ibtt_stream_t &dm_sts, uint64_t* size) {
    #pragma HLS dataflow
    hls::stream<TransferElement> pendingTransfers;
    hls::stream<snap_bool_t> streamIsTerminated;

    {
        TransferElement currentTransfer;
        do {
            in >> currentTransfer;
            uint64_t address = currentTransfer.data.address;
            if (currentTransfer.data.type == AddressType::CardDRAM || currentTransfer.data.type == AddressType::NVMe) {
                address += DRAMBaseOffset;
            }

            if (!streamIsTerminated.read()) {
                issue_block_transfer_command(currentTransfer.data.size, currentTransfer.last, address, dm_cmd);
            }

            pendingTransfers << currentTransfer;
        } while (!currentTransfer.last);
    }
    {
        TransferElement currentTransfer;
        snap_bool_t endOfPacket = false;
        uint64_t tmpSize = 0;
        do {
            // Allow or deny the next transfer
            streamIsTerminated << endOfPacket;

            ap_wait();  // Tick-tock

            pendingTransfers >> currentTransfer;

            if (!endOfPacket) {
                auto result = dm_sts.read();
                tmpSize += result.data(30, 8);
                endOfPacket = result.data[31];
            } else {
                currentTransfer.data.size = 0;
            }
            out << currentTransfer;
        } while (!currentTransfer.last);
        *size = tmpSize;
    }
}

void load_data(const Address &transfer
    , axi_datamover_command_stream_t &dm_cmd
    , axi_datamover_status_stream_t &dm_sts
    , mtl_extmap_t &dram_extentmap
#ifdef NVME_ENABLED
    , mtl_extmap_t &nvme_extentmap
    , NVMeCommandStream &nvme_cmd
    , NVMeResponseStream &nvme_resp
#endif
) {
    hls::stream<TransferElement> partial_transfers, ready_transfers;

#ifdef NVME_ENABLED
    hls::stream<uint64_t> nvme_transfers;
    #pragma HLS stream variable=nvme_transfers depth=16
#endif

#pragma HLS dataflow

    // 1. Perform block mapping
    issue_partial_transfers<TransferType::Read>(transfer, dram_extentmap, partial_transfers
#ifdef NVME_ENABLED
        , nvme_extentmap
        , nvme_transfers
#endif
    );

    // 2. Issue NVMe transfers
    load_nvme_data(partial_transfers, ready_transfers
#ifdef NVME_ENABLED
        , nvme_transfers
        , nvme_cmd
        , nvme_resp
#endif
    );

    // 3. Issue DataMover transfers
    transfer_to_stream(ready_transfers, dm_cmd, dm_sts);
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

        if (config.type == AddressType::Random) {
            random_ctrl[0x10 / sizeof(snapu32_t)] = config.size;
            random_ctrl[0] = 1;  // ap_start = true (self-clearing)
        } else {
            load_data(config
                    , mm2s_cmd
                    , mm2s_sts
                    , dram_read_extmap
                #ifdef NVME_ENABLED
                    , nvme_read_extmap
                    , nvme_read_cmd
                    , nvme_read_resp
                #endif
            );
        }
}

void write_data(const Address &transfer
    , axi_datamover_command_stream_t &dm_cmd
    , axi_datamover_status_ibtt_stream_t &dm_sts
    , mtl_extmap_t &dram_extentmap
#ifdef NVME_ENABLED
    , mtl_extmap_t &nvme_extentmap
    , NVMeCommandStream &nvme_cmd
    , NVMeResponseStream &nvme_resp
#endif
    , uint64_t *size
) {
    hls::stream<TransferElement> partial_transfers, ready_transfers;

#ifdef NVME_ENABLED
    hls::stream<uint64_t> nvme_transfers;
    #pragma HLS stream variable=nvme_transfers depth=16
#endif

#pragma HLS dataflow

    // 1. Perform block mapping
    issue_partial_transfers<TransferType::Write>(transfer, dram_extentmap, partial_transfers
#ifdef NVME_ENABLED
        , nvme_extentmap
        , nvme_transfers
#endif
    );

    // 3. Issue DataMover transfers
    transfer_from_stream(partial_transfers, ready_transfers, dm_cmd, dm_sts, size);

    // 2. Issue NVMe transfers
    write_nvme_data(ready_transfers
#ifdef NVME_ENABLED
        , nvme_transfers
        , nvme_cmd
        , nvme_resp
#endif
    );
}


uint64_t op_mem_write(
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
#ifdef NVME_ENABLED
    NVMeCommandStream &nvme_write_cmd,
    NVMeResponseStream &nvme_write_resp,
#endif
    Address &config) {

    uint64_t size = 0;

    switch (config.type) {
        case AddressType::Host:
        case AddressType::CardDRAM:
        case AddressType::NVMe: {
            write_data(config
                    , s2mm_cmd
                    , s2mm_sts
                    , dram_write_extmap
                #ifdef NVME_ENABLED
                    , nvme_write_extmap
                    , nvme_write_cmd
                    , nvme_write_resp
                #endif
                    , &size
            );
            break;
        }
        default: break;
    }

    return size;
}

}  // namespace fpga
}  // namespace metal
