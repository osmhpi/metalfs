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
  // Common API
 public:
 protected:
  void configure(SnapAction &action, uint64_t inputSize, bool initial) override;
  void finalize(SnapAction &action, uint64_t outputSize,
                bool endOfInput) override;

  std::vector<mtl_file_extent> _extents;

  // API to be used from PipelineStorage (extent list-based)
 public:
  explicit FileDataSinkContext(fpga::AddressType resource, fpga::MapType map,
                                  std::vector<mtl_file_extent> &extents,
                                  uint64_t offset, uint64_t size);

  // API to be used when building file pipelines (filename-based)
 public:
  explicit FileDataSinkContext(std::shared_ptr<PipelineStorage> filesystem,
                                  std::string filename, uint64_t offset,
                                  uint64_t size);

  void prepareForTotalSize(uint64_t size);

 protected:
  void loadExtents();

  std::string _filename;
  std::shared_ptr<PipelineStorage> _filesystem;
  uint64_t _cachedTotalSize;
};

}  // namespace metal
