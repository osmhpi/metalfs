#include <metal_pipeline/data_source.hpp>

namespace metal {

class FileDataSource : public CardMemoryDataSource {
public:
    explicit FileDataSource(std::string filename) : CardMemoryDataSource(0, 0), _filename(filename), _file_offset(0) {}

    uint64_t file_size() const;

protected:
    std::string _filename;
    uint64_t _file_offset;
};

} // namespace metal