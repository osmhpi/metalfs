extern "C" {
#include <metal-filesystem/metal.h>
#include <unistd.h>
}

#include <utility>

#include <spdlog/spdlog.h>

#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>
#include <metal-pipeline/fpga_interface.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

FileDataSinkContext::FileDataSinkContext(
    std::shared_ptr<PipelineStorage> filesystem, uint64_t inode_id,
    uint64_t offset, uint64_t size, bool truncateOnFinalize)
    : DefaultDataSinkContext(
          DataSink(offset, size,
                   filesystem ? filesystem->type() : fpga::AddressType::Host,
                   filesystem ? filesystem->map() : fpga::MapType::None)),
      _inode_id(inode_id),
      _truncateOnFinalize(truncateOnFinalize),
      _filesystem(filesystem),
      _cachedTotalSize(0) {
  if (_inode_id == 0) {
    // 'Disabled' mode
    return;
  }

  loadExtents();
  prepareForTotalSize(offset + size);
}

void FileDataSinkContext::prepareForTotalSize(uint64_t size) {
  if (size <= _cachedTotalSize) {
    return;
  }

  int res;

  res = mtl_truncate(_filesystem->context(), _inode_id, size);
  if (res != MTL_SUCCESS)
    throw std::runtime_error("Unable to update file length");

  loadExtents();
}

void FileDataSinkContext::configure(SnapAction &action, uint64_t inputSize,
                                    bool) {
  _dataSink = _dataSink.withSize(inputSize);

  if (_dataSink.address().addr + _dataSink.address().size > _cachedTotalSize) {
    prepareForTotalSize(_dataSink.address().addr + _dataSink.address().size);
  }

  // TODO: There is a potential race condition when the file metadata was
  // modified between obtaining the extent list and calling configure()

  // Transfer extent list
  switch (_dataSink.address().map) {
    case fpga::MapType::DRAMAndNVMe: {
      auto dramFilesystem = _filesystem->dramPipelineStorage();
      if (dramFilesystem == nullptr) {
        throw std::runtime_error("A DRAM filesystem must be provided.");
      }

      uint64_t pagefileInode;
      if (mtl_open(dramFilesystem->context(),
                   PipelineStorage::PagefileWritePath.c_str(),
                   &pagefileInode) != MTL_SUCCESS) {
        throw std::runtime_error("Pagefile does not exist.");
      }

      std::vector<mtl_file_extent> pagefileExtents(MTL_MAX_EXTENTS);
      uint64_t extents_length, file_length;
      if (mtl_load_extent_list(dramFilesystem->context(), pagefileInode,
                               pagefileExtents.data(), &extents_length,
                               &file_length) != MTL_SUCCESS) {
        throw std::runtime_error("Unable to load pagefile extents.");
      }
      pagefileExtents.resize(extents_length);

      mapExtents(action, fpga::ExtmapSlot::CardDRAMWrite, pagefileExtents);
      mapExtents(action, fpga::ExtmapSlot::NVMeWrite, _extents);
      break;
    }
    case fpga::MapType::DRAM:
      mapExtents(action, fpga::ExtmapSlot::CardDRAMWrite, _extents);
      break;
    case fpga::MapType::NVMe:
      mapExtents(action, fpga::ExtmapSlot::NVMeWrite, _extents);
      break;
    case fpga::MapType::None:
      break;
  }
}

void FileDataSinkContext::finalize(SnapAction &, uint64_t outputSize,
                                   bool endOfInput) {
  // Advance offset
  _dataSink =
      DataSink(_dataSink.address().addr + outputSize, _dataSink.address().size,
               _dataSink.address().type, _dataSink.address().map);

  if (endOfInput) {
    if (_inode_id && _truncateOnFinalize) {
      spdlog::trace("Truncating file to size {}", _dataSink.address().addr);
      int res = mtl_truncate(_filesystem->context(), _inode_id,
                             _dataSink.address().addr);
      if (res != MTL_SUCCESS)
        throw std::runtime_error("Unable to update file length");
    }
  }
}

void FileDataSinkContext::loadExtents() {
  std::vector<mtl_file_extent> extents(MTL_MAX_EXTENTS);
  uint64_t extentsLength, fileLength;
  if (mtl_load_extent_list(_filesystem->context(), _inode_id, extents.data(),
                           &extentsLength, &fileLength) != MTL_SUCCESS) {
    throw std::runtime_error("Unable to load extents");
  }

  extents.resize(extentsLength);

  _extents = extents;
  _cachedTotalSize = fileLength;
}

void FileDataSinkContext::mapExtents(SnapAction &action, fpga::ExtmapSlot slot,
                                     std::vector<mtl_file_extent> &extents) {
  auto *job_struct = reinterpret_cast<uint64_t *>(action.allocateMemory(
      sizeof(uint64_t) *
      (8                                // words for the prefix
       + (2 * fpga::MaxExtentsPerFile)  // two words for each extent
       )));
  job_struct[0] = htobe64(static_cast<uint64_t>(slot));  // slot number
  spdlog::trace("Mapping {} extents for writing", extents.size());

  for (uint64_t i = 0; i < fpga::MaxExtentsPerFile; ++i) {
    if (i < extents.size()) {
      job_struct[8 + 2 * i + 0] = htobe64(extents[i].offset);
      job_struct[8 + 2 * i + 1] = htobe64(extents[i].length);
      spdlog::trace("  Offset {}  Length {}", extents[i].offset,
                    extents[i].length);
    } else {
      job_struct[8 + 2 * i + 0] = 0;
      job_struct[8 + 2 * i + 1] = 0;
    }
  }

  try {
    action.executeJob(fpga::JobType::Map, reinterpret_cast<char *>(job_struct));
  } catch (std::exception &ex) {
    free(job_struct);
    throw ex;
  }

  free(job_struct);
}

}  // namespace metal
