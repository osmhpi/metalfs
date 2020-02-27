#include <metal-filesystem-pipeline/filesystem_context.hpp>

namespace metal {

FilesystemContext::FilesystemContext(std::string metadataDir,
                                     mtl_storage_backend *storage) {
  mtl_initialize(&_context, metadataDir.c_str(), storage);
}

}  // namespace metal
