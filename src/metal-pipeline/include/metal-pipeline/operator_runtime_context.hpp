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

  void set_profiling_enabled(bool enabled) { _profilingEnabled = enabled; }

 protected:
  bool _profilingEnabled{false};
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

class METAL_PIPELINE_API DataSourceRuntimeContext
    : public OperatorRuntimeContext {
 public:
  explicit DataSourceRuntimeContext(DataSource dataSource) : _dataSource(dataSource) {}

  const DataSource &dataSource() const { return _dataSource; }

 protected:
  DataSource _dataSource;
};

class METAL_PIPELINE_API DataSinkRuntimeContext
    : public OperatorRuntimeContext {
 public:
  explicit DataSinkRuntimeContext(DataSink dataSink) : _dataSink(dataSink) {}

  const DataSink &dataSink() const { return _dataSink; }

 protected:
  DataSink _dataSink;
};

}  // namespace metal
