#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>

#include <cstring>
#include <functional>

#include <snap_action_metal.h>
#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_pipeline_runner.hpp>

namespace metal {

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
  FileDataSourceContext source(fpga::AddressType::NVMe, fpga::MapType::NVMe,
                               _read_extents, offset, length);
  DefaultDataSinkContext sink(DataSink(buffer, length));

  // TODO: Surround with try/catch
  SnapPipelineRunner runner(0);
  runner.run(source, sink);

  return MTL_SUCCESS;
}

int PipelineStorage::write(uint64_t offset, const void *buffer,
                           uint64_t length) {
  DefaultDataSourceContext source(DataSource(buffer, length));
  FileDataSinkContext sink(fpga::AddressType::NVMe, fpga::MapType::NVMe,
                           _write_extents, offset, length);

  // TODO: Surround with try/catch
  SnapPipelineRunner runner(0);
  runner.run(source, sink);

  return MTL_SUCCESS;
}

mtl_storage_backend PipelineStorage::backend() {
  return mtl_storage_backend{
      []() { return PipelineStorage::instance().initialize(); },
      []() { return PipelineStorage::instance().deinitialize(); },

      [](mtl_storage_metadata *metadata) {
        return PipelineStorage::instance().mtl_storage_get_metadata(metadata);
      },

      [](const mtl_file_extent *extents, uint64_t length) {
        return PipelineStorage::instance().set_active_read_extent_list(extents,
                                                                       length);
      },
      [](const mtl_file_extent *extents, uint64_t length) {
        return PipelineStorage::instance().set_active_write_extent_list(extents,
                                                                        length);
      },

      [](uint64_t offset, const void *buffer, uint64_t length) {
        return PipelineStorage::instance().write(offset, buffer, length);
      },
      [](uint64_t offset, void *buffer, uint64_t length) {
        return PipelineStorage::instance().read(offset, buffer, length);
      }};
}

int PipelineStorage::initialize() { return MTL_SUCCESS; }

int PipelineStorage::deinitialize() { return MTL_SUCCESS; }

PipelineStorage &PipelineStorage::instance() {
  static PipelineStorage instance;
  return instance;
}

}  // namespace metal
