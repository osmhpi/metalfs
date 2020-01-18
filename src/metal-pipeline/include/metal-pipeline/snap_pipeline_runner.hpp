#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <utility>
#include <vector>

#include <metal-pipeline/pipeline_definition.hpp>

namespace metal {

// Runs a pipeline one or more times
class METAL_PIPELINE_API SnapPipelineRunner {
 public:
  SnapPipelineRunner(int card, std::shared_ptr<PipelineDefinition> pipeline)
      : _pipeline(std::move(pipeline)), _initialized(false), _card(card) {}
  template <typename... Ts>
  SnapPipelineRunner(Ts... userOperators, int card)
      : SnapPipelineRunner(card, std::make_shared<PipelineDefinition>(
                                     std::move(userOperators)...)) {}

  uint64_t run(DataSource dataSource, DataSink dataSink);
  uint64_t run(DataSourceRuntimeContext &dataSource,
               DataSinkRuntimeContext &dataSink);

  static std::string readImageInfo(int card);

 protected:
  void requireReinitialization() { _initialized = false; }

  virtual void preRun(SnapAction &action, DataSourceRuntimeContext &dataSource,
                      DataSinkRuntimeContext &dataSink, bool initialize) {
    (void)action;
    (void)dataSource;
    (void)dataSink;
    (void)initialize;
  };
  virtual void postRun(SnapAction &action, DataSourceRuntimeContext &dataSource,
                       DataSinkRuntimeContext &dataSink, bool finalize) {
    (void)action;
    (void)dataSource;
    (void)dataSink;
    (void)finalize;
  };

  std::shared_ptr<PipelineDefinition> _pipeline;
  bool _initialized;
  int _card;
};

}  // namespace metal
