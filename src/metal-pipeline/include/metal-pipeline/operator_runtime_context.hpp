#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator_argument.hpp>
#include <metal-pipeline/user_operator.hpp>

namespace metal {

class SnapAction;

class METAL_PIPELINE_API OperatorRuntimeContext {
 public:
  virtual void configure(SnapAction &action) { (void)action; };
  virtual void finalize(SnapAction &action) { (void)action; };

  virtual bool needs_preparation() const { return false; }
  virtual void set_is_prepared() {}
  bool profiling_enabled() const { return _profilingEnabled; }
  std::string profilingResults() const { return _profilingResults; }
  void setProfilingResults(std::string results) {
    _profilingResults = std::move(results);
  }

  void set_profiling_enabled(bool enabled) { _profilingEnabled = enabled; }

 protected:
  bool _profilingEnabled;
  std::string _profilingResults;
};

class METAL_PIPELINE_API UserOperatorRuntimeContext
    : public OperatorRuntimeContext {
 public:
  explicit UserOperatorRuntimeContext(UserOperator op);

  void configure(SnapAction &action) final;
  void finalize(SnapAction &action) final;

  bool needs_preparation() const final;
  virtual void set_is_prepared() final { _is_prepared = true; }
  const UserOperator &userOperator() const { return _op; }

 protected:
  UserOperator _op;
  bool _is_prepared;
};

class METAL_PIPELINE_API DataSourceRuntimeContext {
 public:
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

class METAL_PIPELINE_API DefaultDataSourceRuntimeContext
    : public DataSourceRuntimeContext {
 public:
  explicit DefaultDataSourceRuntimeContext(DataSource dataSource)
      : _dataSource(dataSource) {}

  const DataSource dataSource() const override { return _dataSource; }
  uint64_t reportTotalSize() const override { return 0; }
  bool endOfInput() const override { return true; }

 protected:
  DataSource _dataSource;
};

class METAL_PIPELINE_API DataSinkRuntimeContext {
 public:
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
};

class METAL_PIPELINE_API DefaultDataSinkRuntimeContext
    : public DataSinkRuntimeContext {
 public:
  explicit DefaultDataSinkRuntimeContext(DataSink dataSink)
      : _dataSink(dataSink) {}

  const DataSink dataSink() const override { return _dataSink; }
  void prepareForTotalSize(uint64_t size) override { (void)size; };

 protected:
  DataSink _dataSink;
};

}  // namespace metal
