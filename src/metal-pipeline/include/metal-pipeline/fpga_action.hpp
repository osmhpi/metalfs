#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>

#include <metal-pipeline/fpga_interface.hpp>

namespace metal {

class METAL_PIPELINE_API FpgaAction {
 public:
  virtual ~FpgaAction(){}

  virtual void executeJob(fpga::JobType jobType, const void *parameters = nullptr,
                  fpga::Address source = {}, fpga::Address destination = {},
                  uint64_t directData0 = 0, uint64_t directData1 = 0,
                  uint64_t *directDataOut0 = nullptr,
                  uint64_t *directDataOut1 = nullptr) = 0;
  virtual bool isNVMeEnabled() = 0;

  virtual void *allocateMemory(size_t size) = 0;

  virtual std::string addressTypeToString(fpga::AddressType addressType) = 0;
  virtual std::string mapTypeToString(fpga::MapType mapType) = 0;
};

}  // namespace metal
