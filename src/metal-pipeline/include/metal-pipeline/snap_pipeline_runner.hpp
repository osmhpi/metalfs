#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <utility>
#include <vector>

#include <metal-pipeline/card.hpp>
#include <metal-pipeline/pipeline.hpp>

namespace metal {

class DataSourceContext;
class DataSinkContext;

// Runs a pipeline one or more times
class METAL_PIPELINE_API SnapPipelineRunner {
 public:
  SnapPipelineRunner(Card card, std::shared_ptr<Pipeline> pipeline)
      : _pipeline(std::move(pipeline)), _initialized(false), _card(card) {}
  template <typename... Ts>
  SnapPipelineRunner(Card card, Ts... userOperators)
      : SnapPipelineRunner(
            card, std::make_shared<Pipeline>(std::move(userOperators)...)) {}

  std::pair<uint64_t, bool> run(DataSource dataSource, DataSink dataSink);
  std::pair<uint64_t, bool> run(DataSourceContext &dataSource,
                                DataSinkContext &dataSink);

  static std::string readImageInfo(int card);

 protected:
  void requireReinitialization() { _initialized = false; }

  virtual void preRun(SnapAction &action, DataSourceContext &dataSource,
                      DataSinkContext &dataSink, bool initialize) {
    (void)action;
    (void)dataSource;
    (void)dataSink;
    (void)initialize;
  };
  virtual void postRun(SnapAction &action, DataSourceContext &dataSource,
                       DataSinkContext &dataSink, bool finalize) {
    (void)action;
    (void)dataSource;
    (void)dataSink;
    (void)finalize;
  };

  std::shared_ptr<Pipeline> _pipeline;
  bool _initialized;
  Card _card;
};

}  // namespace metal
