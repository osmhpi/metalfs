#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

#include <map>
#include <vector>

#include <metal-filesystem/metal.h>
#include <metal-pipeline/fpga_interface.hpp>

namespace metal {

class METAL_FILESYSTEM_PIPELINE_API PipelineStorage {
 public:
  template <fpga::AddressType TAddress, fpga::MapType TMap>
  static mtl_storage_backend backend();

 protected:
  static PipelineStorage &instance();

  int initialize();
  int deinitialize();

  int mtl_storage_get_metadata(fpga::AddressType addressType, fpga::MapType map,
                               mtl_storage_metadata *metadata);
  int set_active_read_extent_list(fpga::AddressType addressType,
                                  fpga::MapType map,
                                  const mtl_file_extent *extents,
                                  uint64_t length);
  int set_active_write_extent_list(fpga::AddressType addressType,
                                   fpga::MapType map,
                                   const mtl_file_extent *extents,
                                   uint64_t length);
  int read(fpga::AddressType addressType, fpga::MapType map, uint64_t offset,
           void *buffer, uint64_t length);
  int write(fpga::AddressType addressType, fpga::MapType map, uint64_t offset,
            const void *buffer, uint64_t length);

  std::map<std::pair<fpga::AddressType, fpga::MapType>,
           std::vector<mtl_file_extent>>
      _read_extents;
  std::map<std::pair<fpga::AddressType, fpga::MapType>,
           std::vector<mtl_file_extent>>
      _write_extents;
};

template <fpga::AddressType TAddress, fpga::MapType TMap>
mtl_storage_backend PipelineStorage::backend() {
  return mtl_storage_backend{
      []() { return PipelineStorage::instance().initialize(); },
      []() { return PipelineStorage::instance().deinitialize(); },

      [](mtl_storage_metadata *metadata) {
        return PipelineStorage::instance().mtl_storage_get_metadata(
            TAddress, TMap, metadata);
      },

      [](const mtl_file_extent *extents, uint64_t length) {
        return PipelineStorage::instance().set_active_read_extent_list(
            TAddress, TMap, extents, length);
      },
      [](const mtl_file_extent *extents, uint64_t length) {
        return PipelineStorage::instance().set_active_write_extent_list(
            TAddress, TMap, extents, length);
      },

      [](uint64_t offset, const void *buffer, uint64_t length) {
        return PipelineStorage::instance().write(TAddress, TMap, offset, buffer,
                                                 length);
      },
      [](uint64_t offset, void *buffer, uint64_t length) {
        return PipelineStorage::instance().read(TAddress, TMap, offset, buffer,
                                                length);
      }};
}

}  // namespace metal
