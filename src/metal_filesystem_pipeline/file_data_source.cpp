
extern "C" {
#include <metal/metal.h>
}

#include "file_data_source.hpp"

namespace metal {

uint64_t metal::FileDataSource::file_size() const {

    mtl_prepare_storage_for_reading(_filename.c_str(), nullptr);
    return 0;
}

} // namespace metal