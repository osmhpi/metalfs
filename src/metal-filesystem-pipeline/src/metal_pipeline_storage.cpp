#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>

#include <cstring>
#include <functional>

#include <spdlog/spdlog.h>

#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-filesystem-pipeline/filesystem_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/fpga_interface.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_pipeline_runner.hpp>

namespace metal {

PipelineStorage::PipelineStorage(
    int card, fpga::AddressType type, fpga::MapType map,
    std::string metadataDir, bool deleteMetadataIfExists,
    std::shared_ptr<PipelineStorage> dramPipelineStorage)
    : FilesystemContext(metadataDir, deleteMetadataIfExists),
      _card(card),
      _type(type),
      _map(map),
      _dramPipelineStorage(dramPipelineStorage) {
  _backend = mtl_storage_backend{
      [](void *storage_context) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->initialize();
      },
      [](void *storage_context) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->deinitialize();
      },

      [](void *storage_context, mtl_storage_metadata *metadata) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->mtl_storage_get_metadata(metadata);
      },

      [](mtl_context *context, void *storage_context, uint64_t inode_id,
         uint64_t offset, const void *buffer, uint64_t length) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->write(inode_id, offset, buffer, length);
      },
      [](mtl_context *context, void *storage_context, uint64_t inode_id,
         uint64_t offset, void *buffer, uint64_t length) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->read(inode_id, offset, buffer, length);
      },
      this};
  mtl_initialize(&_context, metadataDir.c_str(), &_backend);

  // Make sure that the necessary pagefiles are in place
  if (_map == fpga::MapType::DRAMAndNVMe) {
    if (_dramPipelineStorage == nullptr) {
      throw std::runtime_error("A DRAM filesystem must be provided.");
    }

    createDramPagefile(PagefileReadPath);
    createDramPagefile(PagefileWritePath);
  }
}

void PipelineStorage::createDramPagefile(const std::string &pagefilePath) {
  uint64_t pagefile_inode;
  int res = mtl_open(_dramPipelineStorage->context(), pagefilePath.c_str(),
                     &pagefile_inode);
  if (res == MTL_ERROR_NOENTRY) {
    res = mtl_create(_dramPipelineStorage->context(), pagefilePath.c_str(),
                     &pagefile_inode);
    if (res != MTL_SUCCESS) {
      throw std::runtime_error("Could not create pagefile.");
    }
  } else if (res != MTL_SUCCESS) {
    throw std::runtime_error("Could not open pagefile.");
  }

  res = mtl_truncate(_dramPipelineStorage->context(), pagefile_inode,
                     fpga::PagefileSize);
  if (res != MTL_SUCCESS) {
    throw std::runtime_error("Could not resize pagefile.");
  }
}

int PipelineStorage::mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
  if (metadata) {
    // TODO: The number of blocks per NVMe stick can be obtained through the
    // SNAP MMIO interface
    metadata->num_blocks = 64 * 1024 * 1024;
    metadata->block_size = fpga::StorageBlockSize;
  }

  return MTL_SUCCESS;
}

int PipelineStorage::read(uint64_t inode_id, uint64_t offset, void *buffer,
                          uint64_t length) {
  FileDataSourceContext source(shared_from_this(), inode_id, offset, length);
  DefaultDataSinkContext sink(DataSink(buffer, length));

  try {
    SnapPipelineRunner runner(_card);
    runner.run(source, sink);
    return MTL_SUCCESS;
  } catch (std::exception &e) {
    spdlog::error(e.what());
    // For lack of a better error type
    return MTL_ERROR_INVALID_ARGUMENT;
  }
}

int PipelineStorage::write(uint64_t inode_id, uint64_t offset,
                           const void *buffer, uint64_t length) {
  DefaultDataSourceContext source(DataSource(buffer, length));
  FileDataSinkContext sink(shared_from_this(), inode_id, offset, length);

  try {
    SnapPipelineRunner runner(_card);
    runner.run(source, sink);
    return MTL_SUCCESS;
  } catch (std::exception &e) {
    spdlog::error(e.what());
    return MTL_ERROR_INVALID_ARGUMENT;
  }
}

int PipelineStorage::initialize() { return MTL_SUCCESS; }

int PipelineStorage::deinitialize() { return MTL_SUCCESS; }

}  // namespace metal
