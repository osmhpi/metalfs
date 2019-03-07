#pragma once

#include <vector>

#include <metal_filesystem/metal.h>

namespace metal {

class PipelineStorage {
 public:
  static mtl_storage_backend backend();

 protected:
  static PipelineStorage instance();

  int initialize();
  int deinitialize();

  int mtl_storage_get_metadata(mtl_storage_metadata *metadata);
  int set_active_read_extent_list(const mtl_file_extent *extents, uint64_t length);
  int set_active_write_extent_list(const mtl_file_extent *extents, uint64_t length);
  int read(uint64_t offset, void *buffer, uint64_t length);
  int write(uint64_t offset, void *buffer, uint64_t length);

  std::vector<mtl_file_extent> _read_extents;
  std::vector<mtl_file_extent> _write_extents;
};

}  // namespace metal
