#include <utility>

#include <string>
#include <metal_pipeline/data_sink.hpp>

namespace metal {

class FileDataSink : public CardMemoryDataSink {

  // Common API
 public:
 protected:

  void configure(SnapAction &action) override;
  void finalize(SnapAction &action) override;

  std::vector<mtl_file_extent> _extents;
  uint64_t _offset;

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
