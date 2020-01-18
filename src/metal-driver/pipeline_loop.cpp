#include "pipeline_loop.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>

#include <metal-driver-messages/message_header.hpp>
#include <metal-driver-messages/messages.hpp>
#include <metal-filesystem-pipeline/file_data_sink.hpp>
#include <metal-filesystem-pipeline/file_data_source.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/profiling_pipeline_runner.hpp>

#include "agent_data_sink.hpp"
#include "buffer_data_source.hpp"
#include "pseudo_operators.hpp"
#include "registered_agent.hpp"

namespace metal {

void PipelineLoop::run() {
  ProfilingPipelineRunner runner(_card, _pipeline.pipeline);

  // Establish data sources and sinks
  auto singleStagePipeline =
      _pipeline.dataSourceAgent == _pipeline.dataSinkAgent;
  BufferSourceRuntimeContext dataSource(
      _pipeline.dataSourceAgent, _pipeline.pipeline, singleStagePipeline);
  BufferSinkRuntimeContext dataSink(_pipeline.dataSinkAgent, _pipeline.pipeline,
                                    singleStagePipeline);

  if (DatagenOperator::isDatagenAgent(*_pipeline.dataSourceAgent)) {
    auto isProfilingEnabled = DatagenOperator::isProfilingEnabled(*_pipeline.dataSourceAgent);
    if (_pipeline.pipeline->operators().empty()) {
      // Special handling to be able to deliver profiling results back to the agent
      dataSink.setProfilingEnabled(isProfilingEnabled);
    } else {
      dataSource.setProfilingEnabled(isProfilingEnabled);
    }
  }

  // Wait until all idle clients signal ready
  for (const auto &agent : _pipeline.operatorAgents) {
    if (agent == _pipeline.dataSourceAgent || agent == _pipeline.dataSinkAgent)
      continue;
    agent->receiveProcessingRequest();
  }

  for (;;) {
    runner.run(dataSource, dataSink);

    if (dataSource.endOfInput()) {
      break;
    }
  }

  // Send processing responses
  const auto &operators = _pipeline.pipeline->operators();
  auto currentOperator = operators.begin();
  for (const auto &agent : _pipeline.operatorAgents) {
    if (agent == _pipeline.dataSourceAgent || agent == _pipeline.dataSinkAgent)
      continue;

    ProcessingResponse msg;
    msg.set_eof(true);
    msg.set_message(currentOperator->profilingResults());

    agent->sendProcessingResponse(msg);

    ++currentOperator;
  }
}

}  // namespace metal
