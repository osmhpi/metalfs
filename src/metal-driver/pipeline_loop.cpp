#include "pipeline_loop.hpp"

#include <algorithm>

#include <spdlog/spdlog.h>

#include <metal-driver-messages/message_header.hpp>
#include <metal-driver-messages/messages.hpp>
#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/profiling_pipeline_runner.hpp>

#include "agent_data_sink_context.hpp"
#include "agent_data_source_context.hpp"
#include "operator_agent.hpp"
#include "pseudo_operators.hpp"

namespace metal {

void PipelineLoop::run() {
  ProfilingPipelineRunner runner(_card, _pipeline.pipeline);

  // Establish data sources and sinks
  auto singleStagePipeline =
      _pipeline.dataSourceAgent == _pipeline.dataSinkAgent;
  AgentDataSourceContext dataSource(_pipeline.dataSourceAgent,
                                    _pipeline.pipeline, singleStagePipeline);
  AgentDataSinkContext dataSink(_pipeline.dataSinkAgent, _pipeline.pipeline,
                                singleStagePipeline);

  if (DatagenOperator::isDatagenAgent(*_pipeline.dataSourceAgent)) {
    auto isProfilingEnabled =
        DatagenOperator::isProfilingEnabled(*_pipeline.dataSourceAgent);
    if (_pipeline.pipeline->operators().empty()) {
      // Special handling to be able to deliver profiling results back to the
      // agent
      dataSink.setProfilingEnabled(isProfilingEnabled);
    } else {
      dataSource.setProfilingEnabled(isProfilingEnabled);
    }
  } else if (MetalCatOperator::isMetalCatAgent(*_pipeline.dataSourceAgent)) {
    auto isProfilingEnabled =
        MetalCatOperator::isProfilingEnabled(*_pipeline.dataSourceAgent);
    if (_pipeline.pipeline->operators().empty()) {
      // Special handling to be able to deliver profiling results back to the
      // agent
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
    auto [outputSize, endOfInput] = runner.run(dataSource, dataSink);
    (void)outputSize;

    if (endOfInput) {
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
