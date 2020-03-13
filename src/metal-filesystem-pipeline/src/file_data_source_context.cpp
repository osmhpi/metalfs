#include <unistd.h>

#include <utility>

#include <spdlog/spdlog.h>

#include <metal-filesystem/metal.h>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>
#include <metal-pipeline/fpga_interface.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

size_t FileDataSourceContext::reportTotalSize() { return _fileLength; }

FileDataSourceContext::FileDataSourceContext(
    std::shared_ptr<PipelineStorage> filesystem, uint64_t inode_id,
    uint64_t offset, uint64_t size)
    : DefaultDataSourceContext(
          DataSource(offset, size, filesystem->type(), filesystem->map())),
      _inode_id(inode_id),
      _filesystem(filesystem) {
  if (inode_id == 0) {
    // 'Disabled' mode
    return;
  }

  std::vector<mtl_file_extent> extents(MTL_MAX_EXTENTS);
  uint64_t extents_length, fileLength;

  if (mtl_load_extent_list(_filesystem->context(), inode_id, extents.data(),
                           &extents_length, &fileLength) != MTL_SUCCESS) {
    throw std::runtime_error("Unable to load extents");
  }

  extents.resize(extents_length);

  _extents = std::move(extents);
  _fileLength = fileLength;

  // Make sure that the size is not larger than the file
  _dataSource =
      _dataSource.withSize(std::min(offset + size, _fileLength) - offset);
}

void FileDataSourceContext::configure(SnapAction &action, bool) {
  if (_extents.empty())
    throw std::runtime_error("Extents were not initialized");

  // TODO: There is a potential race condition when the file metadata was
  // modified between obtaining the extent list and calling configure()

  // Transfer extent list
  // Transfer extent list
  switch (_dataSource.address().map) {
    case fpga::MapType::DRAMAndNVMe: {
      auto dramFilesystem = _filesystem->dramPipelineStorage();
      if (dramFilesystem == nullptr) {
        throw std::runtime_error("A DRAM filesystem must be provided.");
      }

      uint64_t pagefileInode;
      if (mtl_open(dramFilesystem->context(),
                   PipelineStorage::PagefileReadPath.c_str(),
                   &pagefileInode) != MTL_SUCCESS) {
        throw std::runtime_error("Pagefile does not exist.");
      }

      std::vector<mtl_file_extent> pagefileExtents(MTL_MAX_EXTENTS);
      uint64_t extents_length, file_length;
      if (mtl_load_extent_list(_filesystem->context(), pagefileInode,
                               pagefileExtents.data(), &extents_length,
                               &file_length) != MTL_SUCCESS) {
        throw std::runtime_error("Unable to load pagefile extents.");
      }
      pagefileExtents.resize(extents_length);

      mapExtents(action, fpga::ExtmapSlot::CardDRAMRead, pagefileExtents);
      mapExtents(action, fpga::ExtmapSlot::NVMeRead, _extents);
      break;
    }
    case fpga::MapType::DRAM:
      mapExtents(action, fpga::ExtmapSlot::CardDRAMRead, _extents);
      break;
    case fpga::MapType::NVMe:
      mapExtents(action, fpga::ExtmapSlot::NVMeRead, _extents);
      break;
    case fpga::MapType::None:
      break;
  }
}

void FileDataSourceContext::mapExtents(SnapAction &action,
                                       fpga::ExtmapSlot slot,
                                       std::vector<mtl_file_extent> &extents) {
  auto *job_struct = reinterpret_cast<uint64_t *>(action.allocateMemory(
      sizeof(uint64_t) *
      (8                                // words for the prefix
       + (2 * fpga::MaxExtentsPerFile)  // two words for each extent
       )));
  job_struct[0] = htobe64(static_cast<uint64_t>(slot));  // slot number
  spdlog::trace("Mapping {} extents for reading", extents.size());

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

void FileDataSourceContext::finalize(SnapAction &action) {
  (void)action;
  // Advance offset
  _dataSource =
      DataSource(_dataSource.address().addr + _dataSource.address().size,
                 _dataSource.address().size, _dataSource.address().type,
                 _dataSource.address().map);
}

bool FileDataSourceContext::endOfInput() const {
  return _dataSource.address().addr + _dataSource.address().size >= _fileLength;
}

}  // namespace metal
