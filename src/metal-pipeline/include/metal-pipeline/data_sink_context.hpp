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

class METAL_PIPELINE_API DataSinkContext {
 public:
  DataSinkContext() : _profilingEnabled(false), _profilingResults() {}
  virtual const DataSink dataSink() const = 0;
  virtual void prepareForTotalSize(uint64_t size) = 0;

  virtual void configure(SnapAction &action, bool initial) {
    (void)action;
    (void)initial;
  };
  virtual void finalize(SnapAction &action, uint64_t outputSize,
                        bool endOfInput) {
    (void)action;
    (void)outputSize;
    (void)endOfInput;
  };

  bool profilingEnabled() const { return _profilingEnabled; }
  void setProfilingEnabled(bool enabled) { _profilingEnabled = enabled; }
  void setProfilingResults(std::string results) {
    _profilingResults = std::move(results);
  }

 protected:
  bool _profilingEnabled;
  std::string _profilingResults;
};

class METAL_PIPELINE_API DefaultDataSinkContext
    : public DataSinkContext {
 public:
  explicit DefaultDataSinkContext(DataSink dataSink)
      : _dataSink(dataSink) {}

  const DataSink dataSink() const override { return _dataSink; }
  void prepareForTotalSize(uint64_t size) override { (void)size; };

 protected:
  DataSink _dataSink;
};

}  // namespace metal
