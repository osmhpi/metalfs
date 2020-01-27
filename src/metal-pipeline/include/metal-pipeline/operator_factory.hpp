#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <unordered_map>

#include "operator.hpp"

namespace metal {

class OperatorSpecification;
class SnapAction;

class METAL_PIPELINE_API OperatorFactory {
 public:
  static OperatorFactory fromFPGA(SnapAction &snapAction);
  static OperatorFactory fromManifestString(const std::string &manifest);

  Operator createOperator(std::string id);

  size_t size() const { return _operatorSpecifications.size(); }
  const std::unordered_map<std::string,
                           std::shared_ptr<const OperatorSpecification>>
      &operatorSpecifications() const {
    return _operatorSpecifications;
  }

 protected:
  explicit OperatorFactory(const std::string &image_json);
  std::unordered_map<std::string, std::shared_ptr<const OperatorSpecification>>
      _operatorSpecifications;
};

}  // namespace metal
