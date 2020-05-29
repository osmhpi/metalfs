#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

extern "C" {
#include <metal-filesystem/storage.h>
}

#include <string>
#include <utility>

#include <metal-pipeline/data_sink_context.hpp>

namespace metal {

class PipelineStorage;

class METAL_FILESYSTEM_PIPELINE_API FileDataSinkContext
    : public DefaultDataSinkContext {
 public:
  explicit FileDataSinkContext(std::shared_ptr<PipelineStorage> filesystem,
                               uint64_t inode_id, uint64_t offset,
                               uint64_t size, bool truncateOnFinalize = false);

  void prepareForTotalSize(uint64_t size);

 protected:
  void loadExtents();
  void configure(SnapAction &action, uint64_t inputSize, bool initial) override;
  void finalize(SnapAction &action, uint64_t outputSize,
                bool endOfInput) override;
  void mapExtents(SnapAction &action, fpga::ExtmapSlot slot, std::vector<mtl_file_extent> &extents);

  uint64_t _inode_id;
  bool _truncateOnFinalize;
  std::vector<mtl_file_extent> _extents;
  std::shared_ptr<PipelineStorage> _filesystem;
  uint64_t _cachedTotalSize;
};

}  // namespace metal
