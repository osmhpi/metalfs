#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>

#include <cstring>
#include <functional>

#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-filesystem-pipeline/filesystem_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/fpga_interface.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_pipeline_runner.hpp>

namespace metal {

PipelineStorage::PipelineStorage(fpga::AddressType type, fpga::MapType map,
                                 std::string metadataDir,
                                 bool deleteMetadataIfExists)
    : FilesystemContext(metadataDir, deleteMetadataIfExists), _type(type), _map(map) {
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

      [](void *storage_context, const mtl_file_extent *extents,
         uint64_t length) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->set_active_read_extent_list(extents, length);
      },
      [](void *storage_context, const mtl_file_extent *extents,
         uint64_t length) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->set_active_write_extent_list(extents, length);
      },

      [](void *storage_context, uint64_t offset, const void *buffer,
         uint64_t length) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->write(offset, buffer, length);
      },
      [](void *storage_context, uint64_t offset, void *buffer,
         uint64_t length) {
        auto This = reinterpret_cast<PipelineStorage *>(storage_context);
        return This->read(offset, buffer, length);
      },
      this};
  mtl_initialize(&_context, metadataDir.c_str(), &_backend);
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

int PipelineStorage::set_active_read_extent_list(const mtl_file_extent *extents,
                                                 uint64_t length) {
  _read_extents = std::vector<mtl_file_extent>(extents, extents + length);
  return MTL_SUCCESS;
}

int PipelineStorage::set_active_write_extent_list(
    const mtl_file_extent *extents, uint64_t length) {
  _write_extents = std::vector<mtl_file_extent>(extents, extents + length);
  return MTL_SUCCESS;
}

int PipelineStorage::read(uint64_t offset, void *buffer, uint64_t length) {
  FileDataSourceContext source(_type, _map, _read_extents, offset, length);
  DefaultDataSinkContext sink(DataSink(buffer, length));

  try {
    SnapPipelineRunner runner(_card);
    runner.run(source, sink);
    return MTL_SUCCESS;
  } catch (std::exception &e) {
    // For lack of a better error type
    return MTL_ERROR_INVALID_ARGUMENT;
  }
}

int PipelineStorage::write(uint64_t offset, const void *buffer,
                           uint64_t length) {
  DefaultDataSourceContext source(DataSource(buffer, length));
  FileDataSinkContext sink(_type, _map, _write_extents, offset, length);
  try {
    SnapPipelineRunner runner(_card);
    runner.run(source, sink);
    return MTL_SUCCESS;
  } catch (std::exception &e) {
    return MTL_ERROR_INVALID_ARGUMENT;
  }
}

int PipelineStorage::initialize() { return MTL_SUCCESS; }

int PipelineStorage::deinitialize() { return MTL_SUCCESS; }

}  // namespace metal
