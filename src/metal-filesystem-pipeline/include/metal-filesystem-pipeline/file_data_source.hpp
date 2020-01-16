#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

#include <metal-filesystem/metal.h>

#include <metal-pipeline/operator_runtime_context.hpp>

namespace metal {

class METAL_FILESYSTEM_PIPELINE_API FileSourceRuntimeContext
    : public DefaultDataSourceRuntimeContext {
  // Common API
 public:
  bool endOfInput() const override;
 protected:
  void configure(SnapAction &action, bool initial) override;
  void finalize(SnapAction &action) override;

  uint64_t _file_length;
  std::vector<mtl_file_extent> _extents;

  // API to be used from PipelineStorage (extent list-based)
 public:
  explicit FileSourceRuntimeContext(fpga::AddressType resource,
                                    fpga::MapType map,
                                    std::vector<mtl_file_extent> &extents,
                                    uint64_t offset, uint64_t size);

  // API to be used when building file pipelines (filename-based)
 public:
  explicit FileSourceRuntimeContext(fpga::AddressType resource,
                                    fpga::MapType map, std::string filename,
                                    uint64_t offset, uint64_t size = 0);
  uint64_t reportTotalSize();

 protected:
  uint64_t loadExtents();

  std::string _filename;
};

}  // namespace metal
