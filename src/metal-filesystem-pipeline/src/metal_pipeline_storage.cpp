#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>

#include <cstring>
#include <functional>

#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/fpga_interface.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_pipeline_runner.hpp>

namespace metal {

int PipelineStorage::mtl_storage_get_metadata(fpga::AddressType addressType,
                                              fpga::MapType map,
                                              mtl_storage_metadata *metadata) {
  if (metadata) {
    // TODO: The number of blocks per NVMe stick can be obtained through the
    // SNAP MMIO interface
    metadata->num_blocks = 64 * 1024 * 1024;
    metadata->block_size = fpga::StorageBlockSize;
  }

  return MTL_SUCCESS;
}

int PipelineStorage::set_active_read_extent_list(fpga::AddressType addressType,
                                                 fpga::MapType map,
                                                 const mtl_file_extent *extents,
                                                 uint64_t length) {
  _read_extents[std::make_pair(addressType, map)] =
      std::vector<mtl_file_extent>(extents, extents + length);
  return MTL_SUCCESS;
}

int PipelineStorage::set_active_write_extent_list(
    fpga::AddressType addressType, fpga::MapType map,
    const mtl_file_extent *extents, uint64_t length) {
  _write_extents[std::make_pair(addressType, map)] =
      std::vector<mtl_file_extent>(extents, extents + length);
  return MTL_SUCCESS;
}

int PipelineStorage::read(fpga::AddressType addressType, fpga::MapType map,
                          uint64_t offset, void *buffer, uint64_t length) {
  FileDataSourceContext source(addressType, map,
                               _read_extents[std::make_pair(addressType, map)],
                               offset, length);
  DefaultDataSinkContext sink(DataSink(buffer, length));

  try {
    SnapPipelineRunner runner(0);
    runner.run(source, sink);
    return MTL_SUCCESS;
  } catch (std::exception &e) {
    // For lack of a better error type
    return MTL_ERROR_INVALID_ARGUMENT;
  }
}

int PipelineStorage::write(fpga::AddressType addressType, fpga::MapType map,
                           uint64_t offset, const void *buffer,
                           uint64_t length) {
  DefaultDataSourceContext source(DataSource(buffer, length));
  FileDataSinkContext sink(addressType, map,
                           _write_extents[std::make_pair(addressType, map)],
                           offset, length);

  try {
    SnapPipelineRunner runner(0);
    runner.run(source, sink);
    return MTL_SUCCESS;
  } catch (std::exception &e) {
    return MTL_ERROR_INVALID_ARGUMENT;
  }
}

int PipelineStorage::initialize() { return MTL_SUCCESS; }

int PipelineStorage::deinitialize() { return MTL_SUCCESS; }

PipelineStorage &PipelineStorage::instance() {
  static PipelineStorage instance;
  return instance;
}

}  // namespace metal
