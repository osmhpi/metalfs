#pragma once

#include <metal-filesystem-pipeline/metal-filesystem-pipeline_api.h>

#include <map>
#include <memory>
#include <vector>

#include <metal-filesystem/metal.h>
#include <metal-filesystem-pipeline/filesystem_context.hpp>
#include <metal-pipeline/card.hpp>
#include <metal-pipeline/fpga_interface.hpp>

namespace metal {

class METAL_FILESYSTEM_PIPELINE_API PipelineStorage
    : public FilesystemContext,
      public std::enable_shared_from_this<PipelineStorage> {
 public:
  PipelineStorage(
      Card card, fpga::AddressType type, fpga::MapType map,
      std::string metadataDir, bool deleteMetadataIfExists = false,
      std::shared_ptr<PipelineStorage> dramPipelineStorage = nullptr);

  fpga::AddressType type() const { return _type; };
  fpga::MapType map() const { return _map; };

  std::shared_ptr<PipelineStorage> dramPipelineStorage() const {
    return _dramPipelineStorage;
  }

  inline static const std::string PagefileReadPath = "/.pagefile_read";
  inline static const std::string PagefileWritePath = "/.pagefile_write";

 protected:
  void createDramPagefile(const std::string &pagefilePath);

  int initialize();
  int deinitialize();

  int mtl_storage_get_metadata(mtl_storage_metadata *metadata);
  int read(uint64_t inode_id, uint64_t offset, void *buffer, uint64_t length);
  int write(uint64_t inode_id, uint64_t offset, const void *buffer,
            uint64_t length);

  Card _card;
  mtl_storage_backend _backend;
  fpga::AddressType _type;
  fpga::MapType _map;
  std::shared_ptr<PipelineStorage> _dramPipelineStorage;
};

}  // namespace metal
