#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>
#include <unordered_map>

#include <metal-pipeline/operator_argument.hpp>

namespace metal {

class METAL_PIPELINE_API OperatorSpecification {
 public:
  explicit OperatorSpecification(std::string id, const std::string& manifest);

  std::string id() const { return _id; }
  std::string description() const { return _description; }
  uint8_t streamID() const { return _streamID; }
  bool prepareRequired() const { return _prepareRequired; }
  const std::unordered_map<std::string, OperatorOptionDefinition>
  optionDefinitions() const {
    return _optionDefinitions;
  }

 protected:
  std::string _id;
  std::string _description;
  uint8_t _streamID;
  bool _prepareRequired;
  std::unordered_map<std::string, OperatorOptionDefinition> _optionDefinitions;
};

}  // namespace metal
