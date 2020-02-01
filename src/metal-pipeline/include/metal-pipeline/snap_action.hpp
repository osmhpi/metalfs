#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <libsnap.h>

#include <string>

#include <snap_action_metal.h>

namespace metal {

class METAL_PIPELINE_API SnapAction {
 public:
  explicit SnapAction(int card = 0);
  SnapAction(const SnapAction &other) = delete;
  SnapAction(SnapAction &&other) noexcept;
  virtual ~SnapAction();

  void executeJob(fpga::JobType jobType, const void *parameters = nullptr,
                  fpga::Address source = {}, fpga::Address destination = {},
                  uint64_t directData0 = 0, uint64_t directData1 = 0,
                  uint64_t *directDataOut0 = nullptr,
                  uint64_t *directDataOut1 = nullptr);
  bool isNVMeEnabled();

  static void *allocateMemory(size_t size);

  static std::string addressTypeToString(fpga::AddressType addressType);
  static std::string mapTypeToString(fpga::MapType mapType);

 protected:
  std::string jobTypeToString(fpga::JobType job);
  std::string snapReturnCodeToString(int rc);

  struct snap_action *_action;
  struct snap_card *_card;
};

}  // namespace metal
