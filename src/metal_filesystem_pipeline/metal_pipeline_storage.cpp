#include <metal_pipeline/pipeline_definition.hpp>
#include <metal_pipeline/data_sink.hpp>
#include <functional>
#include <cstring>
#include "metal_pipeline_storage.hpp"
#include "file_data_source.hpp"
#include "file_data_sink.hpp"
#include <metal_pipeline/pipeline_runner.hpp>

namespace metal {

int PipelineStorage::mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
  if (metadata) {
    metadata->num_blocks = 64 * 1024 * 1024;
    metadata->block_size = 4096;
  }

  return MTL_SUCCESS;
}

int PipelineStorage::set_active_read_extent_list(const mtl_file_extent *extents, uint64_t length) {

  _read_extents = std::vector<mtl_file_extent>(extents, extents + length);
  return MTL_SUCCESS;
}

int PipelineStorage::set_active_write_extent_list(const mtl_file_extent *extents, uint64_t length) {

  _write_extents = std::vector<mtl_file_extent>(extents, extents + length);
  return MTL_SUCCESS;
}

int PipelineStorage::read(uint64_t offset, void *buffer, uint64_t length) {

  auto dataSource = std::make_shared<FileDataSource>(_read_extents, offset, length);
  auto dataSink = std::make_shared<HostMemoryDataSink>(buffer, length);

  auto pipeline = std::make_shared<PipelineDefinition>(std::vector<std::shared_ptr<AbstractOperator>>({ dataSource, dataSink }));

  SnapPipelineRunner runner(pipeline, 0);
  runner.run(true);

  return MTL_SUCCESS;
}

int PipelineStorage::write(uint64_t offset, const void *buffer, uint64_t length) {

  auto dataSource = std::make_shared<HostMemoryDataSource>(buffer, length);
  auto dataSink = std::make_shared<FileDataSink>(_write_extents, offset, length);

  auto pipeline = std::make_shared<PipelineDefinition>(
          std::vector<std::shared_ptr<AbstractOperator>>({dataSource, dataSink}));

  SnapPipelineRunner runner(pipeline, 0);
  runner.run(true);

  return MTL_SUCCESS;
}

mtl_storage_backend PipelineStorage::backend() {
  return mtl_storage_backend {
    []() { return PipelineStorage::instance().initialize(); },
    []() { return PipelineStorage::instance().deinitialize(); },

    [](mtl_storage_metadata *metadata) { return PipelineStorage::instance().mtl_storage_get_metadata(metadata); },

    [](const mtl_file_extent *extents, uint64_t length) {
      return PipelineStorage::instance().set_active_read_extent_list(extents, length);
    },
    [](const mtl_file_extent *extents, uint64_t length) {
      return PipelineStorage::instance().set_active_write_extent_list(extents, length);
    },

    [](uint64_t offset, const void *buffer, uint64_t length) { return PipelineStorage::instance().write(offset, buffer, length); },
    [](uint64_t offset, void *buffer, uint64_t length) { return PipelineStorage::instance().read(offset, buffer, length); }
  };
}

int PipelineStorage::initialize() {
  return MTL_SUCCESS;
}

int PipelineStorage::deinitialize() {
  return MTL_SUCCESS;
}

PipelineStorage PipelineStorage::instance() {
  static PipelineStorage instance;
  return instance;
}

}  // namespace metal
