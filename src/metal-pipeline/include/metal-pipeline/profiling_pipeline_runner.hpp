#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <metal-pipeline/snap_pipeline_runner.hpp>

namespace metal {

class METAL_PIPELINE_API ProfilingPipelineRunner : public SnapPipelineRunner {
 public:
  ProfilingPipelineRunner(Card card, std::shared_ptr<Pipeline> pipeline)
      : SnapPipelineRunner(card, std::move(pipeline)),
        _results(ProfilingResults{}) {}
  template <typename... Ts>
  ProfilingPipelineRunner(Card card, Ts... userOperators)
      : ProfilingPipelineRunner(
            card, std::make_shared<Pipeline>(std::move(userOperators)...)) {}

  std::string formatProfilingResults(bool dataSource, bool dataSink);
  void resetResults() { _results = ProfilingResults{}; };

 protected:
  struct ProfilingResults {
    uint64_t globalClockCounter;
    uint64_t inputDataByteCount;
    uint64_t inputTransferCycleCount;
    uint64_t inputSlaveIdleCount;
    uint64_t inputMasterIdleCount;
    uint64_t outputDataByteCount;
    uint64_t outputTransferCycleCount;
    uint64_t outputSlaveIdleCount;
    uint64_t outputMasterIdleCount;
  };

  void preRun(SnapAction &action, DataSourceContext &dataSource,
              DataSinkContext &dataSink, bool initialize) override;
  void postRun(SnapAction &action, DataSourceContext &dataSource,
               DataSinkContext &dataSink, bool finalize) override;
  template <typename... Args>
  static std::string string_format(const std::string &format, Args... args);

  std::optional<std::pair<uint8_t, uint8_t>> _profileStreamIds;
  ProfilingResults _results;
};

}  // namespace metal
