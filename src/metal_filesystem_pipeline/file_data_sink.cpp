
extern "C" {
#include <metal/metal.h>
}

#include "file_data_sink.hpp"

namespace metal {

void metal::FileDataSink::ensureFileAllocation(uint64_t size) {
    mtl_prepare_storage_for_writing(_filename.c_str(), _file_offset + size);
}

} // namespace metal