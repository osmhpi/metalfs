#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

#include <string>

#include <metal-filesystem/metal.h>
#include <metal-pipeline/fpga_interface.hpp>

namespace metal {

class METAL_FILESYSTEM_PIPELINE_API FilesystemContext {
 public:
  FilesystemContext(std::string metadataDir, mtl_storage_backend *storage);

  mtl_context *context() { return _context; }
  fpga::AddressType type() const { return _type; };
  fpga::MapType map() const { return _map; };

 protected:
  mtl_context *_context;
  fpga::AddressType _type;
  fpga::MapType _map;
};

}  // namespace metal
