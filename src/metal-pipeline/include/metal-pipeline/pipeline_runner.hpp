#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <metal-pipeline/pipeline_definition.hpp>
#include <utility>
#include <vector>

namespace metal {

// Runs a pipeline one or more times
class METAL_PIPELINE_API SnapPipelineRunner {
 public:
  SnapPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline, int card)
      : _pipeline(std::move(pipeline)),
        _initialized(false),
        _card(card) {}
  template <typename... Ts>
  SnapPipelineRunner(Ts... userOperators, int card)
      : SnapPipelineRunner(std::make_shared<PipelineDefinition>({userOperators...}), card) {}

  uint64_t run(DataSourceRuntimeContext &dataSource, DataSinkRuntimeContext &dataSink, bool last);

  static std::string readImageInfo(int card);

 protected:
  void requireReinitialization() { _initialized = false; }

  virtual void pre_run(SnapAction &action, bool initialize) {};
  virtual void post_run(SnapAction &action, bool finalize) {};

  std::shared_ptr<PipelineDefinition> _pipeline;
  bool _initialized;
  int _card;
};

class METAL_PIPELINE_API ProfilingPipelineRunner : public SnapPipelineRunner {
 public:
  ProfilingPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline,
                          int card)
      : SnapPipelineRunner(std::move(pipeline), card),
        _results(ProfilingResults{}) {}
  template <typename... Ts>
  ProfilingPipelineRunner(Ts... userOperators, int card)
      : ProfilingPipelineRunner(std::make_shared<PipelineDefinition>({userOperators...}), card) {}

  void selectOperatorForProfiling(const std::string &operatorId);
  std::string formatProfilingResults();
  void resetResults() { _results = ProfilingResults{}; };

 protected:
  struct ProfilingResults {
    uint64_t global_clock_counter;
    uint64_t input_data_byte_count;
    uint64_t input_transfer_cycle_count;
    uint64_t input_slave_idle_count;
    uint64_t input_master_idle_count;
    uint64_t output_data_byte_count;
    uint64_t output_transfer_cycle_count;
    uint64_t output_slave_idle_count;
    uint64_t output_master_idle_count;
  };

  void pre_run(SnapAction &action, bool initialize) override;
  void post_run(SnapAction &action, bool finalize) override;
  template <typename... Args>
  static std::string string_format(const std::string &format, Args... args);

  ProfilingResults _results;
};

}  // namespace metal
