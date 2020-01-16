#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>
#include <unordered_map>
#include <vector>
#include "operator_argument.hpp"

namespace metal {

class SnapAction;

class METAL_PIPELINE_API AbstractOperator {
 public:
  virtual void configure(SnapAction &action) { (void)action; };
  virtual void finalize(SnapAction &action) { (void)action; };

  virtual std::string id() const = 0;
  virtual std::string description() const { return ""; }
  virtual uint8_t internal_id() const = 0;
  virtual bool needs_preparation() const { return false; }
  virtual void set_is_prepared() {}
  bool profiling_enabled() const { return _profilingEnabled; }

  std::unordered_map<std::string, OperatorOptionDefinition>
      &optionDefinitions() {
    return _optionDefinitions;
  }

  void setOption(std::string option, OperatorArgumentValue arg);
  void set_profiling_enabled(bool enabled) { _profilingEnabled = enabled; }

 protected:
  bool _profilingEnabled{false};

  std::unordered_map<std::string, std::optional<OperatorArgumentValue>>
      _options;
  std::unordered_map<std::string, OperatorOptionDefinition> _optionDefinitions;
};

}  // namespace metal
