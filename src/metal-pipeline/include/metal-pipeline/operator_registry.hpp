#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <unordered_map>
#include "user_operator.hpp"

namespace metal {

class METAL_PIPELINE_API OperatorRegistry {
 public:
  explicit OperatorRegistry(const std::string &image_json);

  void add_operator(std::string id, std::shared_ptr<AbstractOperator> op) {
    _operators.emplace(std::make_pair(std::move(id), std::move(op)));
  }
  const std::unordered_map<std::string, std::shared_ptr<AbstractOperator>>
      &operators() const {
    return _operators;
  }

 protected:
  std::unordered_map<std::string, std::shared_ptr<AbstractOperator>> _operators;
};

}  // namespace metal
