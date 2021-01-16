#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <vector>

#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator_context.hpp>

namespace metal {

class DataSource;
class DataSink;
class Operator;

/**
*  @brief
*    Defines an Operator execution plan which can be executed in combination
*    with an FpgaAction.
*/
class METAL_PIPELINE_API Pipeline {
 public:
  explicit Pipeline(std::vector<Operator> operators);
  explicit Pipeline(std::vector<OperatorContext> operators);

  Pipeline() : Pipeline(std::vector<OperatorContext>()) {}

  template <typename T, typename... Ts>
  Pipeline(T firstOperator, Ts... rest)
      : Pipeline(std::apply(
            [](auto &&... elems) {  // Turning the parameters into an array...
              std::vector<T> result;
              result.reserve(sizeof...(elems));
              (result.emplace_back(std::forward<decltype(elems)>(elems)), ...);
              return result;
            },
            std::make_tuple(std::move(firstOperator), std::move(rest)...))) {}

  std::vector<OperatorContext> &operators() { return _operators; }

  uint64_t run(DataSource dataSource, DataSink dataSink, FpgaAction &action);

  void configureSwitch(FpgaAction &action, bool set_cached);

 protected:
  std::vector<OperatorContext> _operators;
  bool _cachedSwitchConfiguration;
};

}  // namespace metal
