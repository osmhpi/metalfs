#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

#include <string>

#include <metal-filesystem/metal.h>
#include <metal-pipeline/fpga_interface.hpp>

namespace metal {

class METAL_FILESYSTEM_PIPELINE_API FilesystemContext {
 public:
  FilesystemContext(std::string metadataDir,
                    bool deleteMetadataIfExists = false);
  virtual ~FilesystemContext() = default;

  mtl_context *context() { return _context; }

 protected:
  mtl_context *_context;
};

}  // namespace metal
