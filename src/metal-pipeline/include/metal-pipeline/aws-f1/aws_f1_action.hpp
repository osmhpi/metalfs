#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>

#include <metal-pipeline/fpga_action.hpp>
#include <metal-pipeline/fpga_action_factory.hpp>

namespace metal {

class METAL_PIPELINE_API AwsF1ActionFactory : public FpgaActionFactory {
public:
  AwsF1ActionFactory(int card, int timeout) : FpgaActionFactory(), _card(card), _timeout(timeout) {}
  std::unique_ptr<FpgaAction> createAction() const override;

protected:
  int _card;
  int _timeout;
};

class METAL_PIPELINE_API AwsF1Action : public FpgaAction{
 public:
  explicit AwsF1Action(int card = 0, int timeout = 10);
  AwsF1Action(const AwsF1Action &other) = delete;
  AwsF1Action(AwsF1Action &&other) noexcept;
  virtual ~AwsF1Action();

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

  int _timeout;
};

}  // namespace metal
