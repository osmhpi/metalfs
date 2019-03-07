#include <utility>


extern "C" {
#include <metal_filesystem/metal.h>
}

#include "file_data_source.hpp"

namespace metal {

//uint64_t metal::FileDataSource::file_size() const {
//
//    // Resolve extent list
//    mtl_prepare_storage_for_reading(_filename.c_str(), nullptr);
//    return 0;
//}

FileDataSource::FileDataSource(std::string filename, uint64_t offset, uint64_t length)
        : CardMemoryDataSource(0, 0), _filename(std::move(filename)), _file_offset(0) {

}

FileDataSource::FileDataSource(std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t length)
        : CardMemoryDataSource(0, 0) {

}

} // namespace metal
