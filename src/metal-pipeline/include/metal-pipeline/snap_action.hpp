#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>

#include <metal-pipeline/card.hpp>
#include <metal-pipeline/fpga_interface.hpp>

struct snap_action;
struct snap_card;

namespace metal {

class METAL_PIPELINE_API SnapAction {
 public:
  explicit SnapAction(Card card = {0, 10});
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
  static std::string jobTypeToString(fpga::JobType job);
  static std::string snapReturnCodeToString(int rc);

  struct snap_action *_action;
  struct snap_card *_card;

  int _timeout;
};

}  // namespace metal
