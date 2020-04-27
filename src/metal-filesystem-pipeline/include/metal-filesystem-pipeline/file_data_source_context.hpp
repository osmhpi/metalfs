#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

#include <metal-filesystem/metal.h>
#include <metal-pipeline/data_source_context.hpp>

namespace metal {

class PipelineStorage;

class METAL_FILESYSTEM_PIPELINE_API FileDataSourceContext
    : public DefaultDataSourceContext {
 public:
  explicit FileDataSourceContext(std::shared_ptr<PipelineStorage> filesystem,
                                 uint64_t inode_id, uint64_t offset,
                                 uint64_t size = 0);
  uint64_t reportTotalSize();
  bool endOfInput() const override;

 protected:
  uint64_t loadExtents();
  void configure(SnapAction &action, bool initial) override;
  void finalize(SnapAction &action) override;
  void mapExtents(SnapAction &action, fpga::ExtmapSlot slot, std::vector<mtl_file_extent> &extents);

  uint64_t _inode_id;
  std::shared_ptr<PipelineStorage> _filesystem;

  uint64_t _fileLength;
  std::vector<mtl_file_extent> _extents;
};

}  // namespace metal
