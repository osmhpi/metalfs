#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator.hpp>
#include <metal-pipeline/operator_argument.hpp>

namespace metal {

class SnapAction;

class METAL_PIPELINE_API DataSourceContext {
 public:
  DataSourceContext() : _profilingEnabled(false), _profilingResults() {}
  virtual void configure(SnapAction &action, bool initial) {
    (void)action;
    (void)initial;
  };
  virtual void finalize(SnapAction &action) { (void)action; };

  virtual const DataSource dataSource() const = 0;
  virtual uint64_t reportTotalSize() const = 0;
  virtual bool endOfInput() const = 0;
  bool profilingEnabled() const { return _profilingEnabled; }
  void setProfilingEnabled(bool enabled) { _profilingEnabled = enabled; }
  void setProfilingResults(std::string results) {
    _profilingResults = std::move(results);
  }

 protected:
  bool _profilingEnabled;
  std::string _profilingResults;
};

class METAL_PIPELINE_API DefaultDataSourceContext : public DataSourceContext {
 public:
  explicit DefaultDataSourceContext(DataSource dataSource)
      : _dataSource(dataSource) {}

  const DataSource dataSource() const override { return _dataSource; }
  uint64_t reportTotalSize() const override { return 0; }
  bool endOfInput() const override { return true; }

 protected:
  DataSource _dataSource;
};

}  // namespace metal
