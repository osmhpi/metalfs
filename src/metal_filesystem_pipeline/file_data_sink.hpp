#include <utility>

#include <string>
#include <metal_pipeline/data_sink.hpp>

namespace metal {

class FileDataSink : public CardMemoryDataSink {
public:
    explicit FileDataSink(std::string filename) : CardMemoryDataSink(0, 0), _filename(std::move(filename)), _file_offset(0) {}

    void ensureFileAllocation(uint64_t size);

protected:
    std::string _filename;
    uint64_t _file_offset;
};

} // namespace metal