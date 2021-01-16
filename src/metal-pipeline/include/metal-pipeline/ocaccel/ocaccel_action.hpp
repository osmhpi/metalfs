#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>

#include <metal-pipeline/fpga_action.hpp>
#include <metal-pipeline/fpga_action_factory.hpp>

struct snap_action;
struct snap_card;

namespace metal {

class METAL_PIPELINE_API OCAccelActionFactory : public FpgaActionFactory {
public:
  OCAccelActionFactory(int card, int timeout) : FpgaActionFactory(), _card(card), _timeout(timeout) {}
  std::unique_ptr<FpgaAction> createAction() const override;

protected:
  int _card;
  int _timeout;
};

class METAL_PIPELINE_API OCAccelAction : public FpgaAction {
 public:
  explicit OCAccelAction(int card = 0, int timeout = 10);
  OCAccelAction(const OCAccelAction &other) = delete;
  OCAccelAction(OCAccelAction &&other) noexcept;
  virtual ~OCAccelAction();

  void executeJob(fpga::JobType jobType, const void *parameters = nullptr,
                  fpga::Address source = {}, fpga::Address destination = {},
                  uint64_t directData0 = 0, uint64_t directData1 = 0,
                  uint64_t *directDataOut0 = nullptr,
                  uint64_t *directDataOut1 = nullptr) override;
  bool isNVMeEnabled() override;

  void *allocateMemory(size_t size) override;

  std::string addressTypeToString(fpga::AddressType addressType) override;
  std::string mapTypeToString(fpga::MapType mapType) override;

 protected:
  static std::string jobTypeToString(fpga::JobType job);
  static std::string snapReturnCodeToString(int rc);

  struct snap_action *_action;
  struct snap_card *_card;

  int _timeout;
};

}  // namespace metal
