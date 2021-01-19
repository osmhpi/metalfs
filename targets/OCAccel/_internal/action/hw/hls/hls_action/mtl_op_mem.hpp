#pragma once

namespace metal {
namespace fpga {

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


}  // namespace fpga
}  // namespace metal
