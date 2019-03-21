#include <metal_pipeline/data_source.hpp>
#include <metal_filesystem/metal.h>

namespace metal {

#define NVME_BLOCK_BYTES 512

class FileDataSource : public CardMemoryDataSource {

  // Common API
 public:
 protected:

  void configure(SnapAction &action) override;
  void finalize(SnapAction &action) override;

  std::vector<mtl_file_extent> _extents;
  uint64_t _offset;


  // API to be used from PipelineStorage (extent list-based)
 public:
  explicit FileDataSource(std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t size);


  // API to be used when building file pipelines (filename-based)
 public:
  explicit FileDataSource(std::string filename, uint64_t offset, uint64_t size = 0);

 protected:
  void loadExtents();

  std::string _filename;

};

} // namespace metal
