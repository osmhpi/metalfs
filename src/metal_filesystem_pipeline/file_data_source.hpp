#include <metal_pipeline/data_source.hpp>
#include <metal_filesystem/metal.h>

namespace metal {

class FileDataSource : public CardMemoryDataSource {
public:
    explicit FileDataSource(std::string filename, uint64_t offset, uint64_t length);
    explicit FileDataSource(std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t length);

protected:
    std::string _filename;
    uint64_t _file_offset;
};

} // namespace metal
