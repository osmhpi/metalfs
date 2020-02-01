#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "operator_argument.hpp"

namespace metal {

class OperatorSpecification;

class METAL_PIPELINE_API Operator {
  std::shared_ptr<const OperatorSpecification> _spec;

 public:
  explicit Operator(std::shared_ptr<const OperatorSpecification> spec);
  Operator(Operator &&other)
      : _spec(std::move(other._spec)), _options(std::move(other._options)){};
  std::string id() const;
  std::string description() const;

  const std::unordered_map<std::string, std::optional<OperatorArgumentValue>>
  options() const {
    return _options;
  }

  const OperatorSpecification &spec() const { return *_spec; }

  void setOption(std::string option, OperatorArgumentValue arg);

 protected:
  std::unordered_map<std::string, std::optional<OperatorArgumentValue>>
      _options;
};

}  // namespace metal
