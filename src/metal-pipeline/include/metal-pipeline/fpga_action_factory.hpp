#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>

#include <metal-pipeline/ocaccel_action.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

class FpgaAction;

class METAL_PIPELINE_API FpgaActionFactory {
public:
  virtual ~FpgaActionFactory() {}
  virtual std::unique_ptr<FpgaAction> createAction() const = 0;
};

class METAL_PIPELINE_API SnapActionFactory : public FpgaActionFactory {
public:
  SnapActionFactory(int card, int timeout) : FpgaActionFactory(), _card(card), _timeout(timeout) {}
  std::unique_ptr<FpgaAction> createAction() const override;

protected:
  int _card;
  int _timeout;
};

class METAL_PIPELINE_API OCAccelActionFactory : public FpgaActionFactory {
public:
  OCAccelActionFactory(int card, int timeout) : FpgaActionFactory(), _card(card), _timeout(timeout) {}
  std::unique_ptr<FpgaAction> createAction() const override;

protected:
  int _card;
  int _timeout;
};

}  // namespace metal
