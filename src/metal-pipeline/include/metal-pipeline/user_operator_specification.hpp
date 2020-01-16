#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>
#include <unordered_map>

#include <metal-pipeline/operator_argument.hpp>

namespace metal {

class METAL_PIPELINE_API UserOperatorSpecification {
 public:
  explicit UserOperatorSpecification(std::string id,
                                     const std::string& manifest);

  std::string id() const { return _id; }
  std::string description() const { return _description; }
  uint8_t internal_id() const { return _internal_id; }
  bool prepare_required() const { return _prepare_required; }
  const std::unordered_map<std::string, OperatorOptionDefinition>
  optionDefinitions() const {
    return _optionDefinitions;
  }

 protected:
  std::string _id;
  std::string _description;
  uint8_t _internal_id;
  bool _prepare_required;
  std::unordered_map<std::string, OperatorOptionDefinition> _optionDefinitions;
};

}  // namespace metal
