#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator_runtime_context.hpp>
#include <vector>

namespace metal {

class DataSource;
class DataSink;
class UserOperator;

class METAL_PIPELINE_API PipelineDefinition {
 public:
  explicit PipelineDefinition(std::vector<UserOperator> userOperators);
  explicit PipelineDefinition(
      std::vector<UserOperatorRuntimeContext> userOperators);

  PipelineDefinition()
      : PipelineDefinition(std::vector<UserOperatorRuntimeContext>()) {}

  template <typename T, typename... Ts>
  PipelineDefinition(T firstOperator, Ts... rest)
      : PipelineDefinition(std::apply(
            [](auto &&... elems) {  // Turning the parameters into an array...
              std::vector<T> result;
              result.reserve(sizeof...(elems));
              (result.emplace_back(std::forward<decltype(elems)>(elems)), ...);
              return result;
            },
            std::make_tuple(std::move(firstOperator), std::move(rest)...))) {}

  std::vector<UserOperatorRuntimeContext> &operators() { return _operators; }

  uint64_t run(DataSource dataSource, DataSink dataSink, SnapAction &action);

  void configureSwitch(SnapAction &action, bool set_cached);

 protected:
  std::vector<UserOperatorRuntimeContext> _operators;
  bool _cached_switch_configuration;
};

}  // namespace metal
