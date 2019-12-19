#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

extern "C" {
#include <metal-filesystem/storage.h>
}

#include <utility>
#include <string>

#include <metal-pipeline/data_sink.hpp>

namespace metal {

class METAL_FILESYSTEM_PIPELINE_API FileDataSink : public DataSink {

  // Common API
public:
protected:
  fpga::AddressType addressType() override { return fpga::AddressType::NVMe; }
  fpga::MapType mapType() override { return fpga::MapType::NVMe; }

  void configure(SnapAction &action) override;
  void finalize(SnapAction &action) override;
  void seek(uint64_t offset) { _address = offset; }

  std::vector<mtl_file_extent> _extents;

  // API to be used from PipelineStorage (extent list-based)
public:
    explicit FileDataSink(std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t size);


  // API to be used when building file pipelines (filename-based)
public:
  explicit FileDataSink(std::string filename, uint64_t offset, uint64_t size = 0);

  void prepareForTotalProcessingSize(size_t size) override;
  void setSize(size_t size) override;

protected:
  void loadExtents();

  std::string _filename;
  uint64_t _cached_total_size;

};

} // namespace metal
