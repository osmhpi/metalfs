#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>

namespace metal {

class FpgaAction;

class METAL_PIPELINE_API FpgaActionFactory {
public:
  virtual ~FpgaActionFactory() {}
  virtual std::unique_ptr<FpgaAction> createAction() const = 0;
};

}  // namespace metal
