#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

#include <map>
#include <vector>

#include <metal-filesystem/metal.h>
#include <metal-filesystem-pipeline/filesystem_context.hpp>
#include <metal-pipeline/fpga_interface.hpp>

namespace metal {

class METAL_FILESYSTEM_PIPELINE_API PipelineStorage : public FilesystemContext {
 public:
  PipelineStorage(fpga::AddressType type, fpga::MapType map,
                  std::string metadataDir, bool deleteMetadataIfExists = false);

  fpga::AddressType type() const { return _type; };
  fpga::MapType map() const { return _map; };

 protected:
  int initialize();
  int deinitialize();

  int mtl_storage_get_metadata(mtl_storage_metadata *metadata);
  int set_active_read_extent_list(const mtl_file_extent *extents,
                                  uint64_t length);
  int set_active_write_extent_list(const mtl_file_extent *extents,
                                   uint64_t length);
  int read(uint64_t offset, void *buffer, uint64_t length);
  int write(uint64_t offset, const void *buffer, uint64_t length);

  mtl_storage_backend _backend;
  fpga::AddressType _type;
  fpga::MapType _map;
  std::vector<mtl_file_extent> _read_extents;
  std::vector<mtl_file_extent> _write_extents;
  int _card;
};

}  // namespace metal
