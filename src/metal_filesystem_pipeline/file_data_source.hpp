#include <metal_pipeline/data_source.hpp>
#include <metal_filesystem/metal.h>

namespace metal {

class FileDataSource : public DataSource {

// Common API
public:
protected:
  fpga::AddressType addressType() override { return fpga::AddressType::NVMe; }

  void configure(SnapAction &action) override;
  void finalize(SnapAction &action) override;
  void seek(uint64_t offset) { _address = offset; }

  std::vector<mtl_file_extent> _extents;


  // API to be used from PipelineStorage (extent list-based)
public:
  explicit FileDataSource(std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t size);


  // API to be used when building file pipelines (filename-based)
public:
  explicit FileDataSource(std::string filename, uint64_t offset, uint64_t size = 0);
  size_t reportTotalSize() override;

 protected:
  uint64_t loadExtents();

  std::string _filename;

};

} // namespace metal
