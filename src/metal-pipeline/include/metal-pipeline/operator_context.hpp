#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator_argument.hpp>
#include <metal-pipeline/operator.hpp>

namespace metal {

class SnapAction;

class METAL_PIPELINE_API OperatorContext {
 public:
  explicit OperatorContext(Operator op);

  void configure(SnapAction &action);
  void finalize(SnapAction &action);

  bool needs_preparation() const;
  void set_is_prepared() { _is_prepared = true; }
  const Operator &userOperator() const { return _op; }

  bool profiling_enabled() const { return _profilingEnabled; }
  void set_profiling_enabled(bool enabled) { _profilingEnabled = enabled; }
  std::string profilingResults() const { return _profilingResults; }
  void setProfilingResults(std::string results) {
    _profilingResults = std::move(results);
  }

 protected:
  Operator _op;
  bool _is_prepared;
  bool _profilingEnabled;
  std::string _profilingResults;
};

}  // namespace metal
