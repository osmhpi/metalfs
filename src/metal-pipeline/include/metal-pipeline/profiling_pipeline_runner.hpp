#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <metal-pipeline/snap_pipeline_runner.hpp>

namespace metal {

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

  void preRun(SnapAction &action, DataSourceRuntimeContext &dataSource,
              DataSinkRuntimeContext &dataSink, bool initialize) override;
  void postRun(SnapAction &action, DataSourceRuntimeContext &dataSource,
               DataSinkRuntimeContext &dataSink, bool finalize) override;
  template <typename... Args>
  static std::string string_format(const std::string &format, Args... args);

  std::optional<std::pair<uint8_t, uint8_t>> _profileStreamIds;
  ProfilingResults _results;
};

}  // namespace metal
