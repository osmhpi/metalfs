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

  virtual void preRun(SnapAction &action, bool initialize) {
    (void)action;
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

class METAL_PIPELINE_API ProfilingPipelineRunner : public SnapPipelineRunner {
 public:
  ProfilingPipelineRunner(int card,
                          std::shared_ptr<PipelineDefinition> pipeline)
      : SnapPipelineRunner(card, std::move(pipeline)),
        _results(ProfilingResults{}) {}
  template <typename... Ts>
  ProfilingPipelineRunner(int card, Ts... userOperators)
      : ProfilingPipelineRunner(card, std::make_shared<PipelineDefinition>(
                                          std::move(userOperators)...)) {}

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

  void preRun(SnapAction &action, bool initialize) override;
  void postRun(SnapAction &action, DataSourceRuntimeContext &dataSource,
               DataSinkRuntimeContext &dataSink,  bool finalize) override;
  template <typename... Args>
  static std::string string_format(const std::string &format, Args... args);

  uint8_t _profileInputStreamId;
  uint8_t _profileOutputStreamId;
  ProfilingResults _results;
};

}  // namespace metal
