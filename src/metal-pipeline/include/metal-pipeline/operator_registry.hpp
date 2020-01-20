#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <unordered_map>

#include "operator.hpp"

namespace metal {

class OperatorSpecification;

class METAL_PIPELINE_API OperatorRegistry {
 public:
  explicit OperatorRegistry(const std::string &image_json);
  Operator createOperator(std::string id);

  void add_operator(std::string id,
                    std::shared_ptr<const OperatorSpecification> op) {
    _operators.emplace(std::make_pair(std::move(id), std::move(op)));
  }
  size_t size() const { return _operators.size(); }
  const std::unordered_map<std::string,
                           std::shared_ptr<const OperatorSpecification>>
      &operatorSpecifications() const {
    return _operators;
  }

 protected:
  std::unordered_map<std::string, std::shared_ptr<const OperatorSpecification>>
      _operators;
};

}  // namespace metal
