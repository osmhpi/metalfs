#include <Messages.pb.h>
#include <metal_frontend/messages/message_header.hpp>
#include <metal_pipeline/data_source.hpp>
#include <metal_pipeline/data_sink.hpp>
#include <metal_pipeline/pipeline_runner.hpp>
#include <algorithm>
#include "pipeline_loop.hpp"
#include "registered_agent.hpp"

namespace metal {

void PipelineLoop::run() {

  // Build a PipelineDefinition
  std::vector<std::shared_ptr<AbstractOperator>> pipeline_operators;
  pipeline_operators.reserve(_pipeline.size());
  for (const auto &operator_agent : _pipeline) {
    pipeline_operators.emplace_back(operator_agent.first);
  }
  auto pipeline_definition = std::make_shared<PipelineDefinition>(std::move(pipeline_operators));

  ProfilingPipelineRunner runner(pipeline_definition, _card);

  auto dataSource = std::dynamic_pointer_cast<DataSource>(_pipeline.front().first);
  auto dataSink = std::dynamic_pointer_cast<DataSink>(_pipeline.back().first);

  // Some data sources (e.g. FileDataSource) are capable of providing the overall processing size in the beginning
  // This might only be a temporary optimization since we will allow operators to provide arbitrary output data sizes
  // in the future.
  if (auto total_size = dataSource->reportTotalSize()) {
    dataSink->prepareForTotalProcessingSize(total_size);
  }

  auto firstAgentIt = std::find_if(_pipeline.cbegin(), _pipeline.cend(),
                                   [](const std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>> &elem) {
                                       return elem.second != nullptr;
                                   });
  assert(firstAgentIt != _pipeline.cend());

  auto lastAgentReverseIt = std::find_if(_pipeline.crbegin(), _pipeline.crend(),
                                         [](const std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>> &elem) {
                                             return elem.second != nullptr;
                                         });
  assert(lastAgentReverseIt != _pipeline.crend());
  auto lastAgentIt = (lastAgentReverseIt+1).base();

    // Receive ready signal from first client
  auto inputBufferMessage = firstAgentIt->second->socket.receive_message<message_type::AGENT_PUSH_BUFFER>();

  // Wait until all other clients signal ready as well
  for (auto operatorAgentPair = std::next(firstAgentIt, 1); operatorAgentPair != std::next(lastAgentIt, 1); ++operatorAgentPair) {
    operatorAgentPair->second->socket.receive_message<message_type::AGENT_PUSH_BUFFER>();
  }

  for (;;) {
    ServerProcessedBuffer processing_response;

    size_t size = 0;
    bool eof = false;

    size_t output_size = 0;

    // If the client provides us with input data, forward it to the pipeline
    if (firstAgentIt->second->input_buffer) {
      size = inputBufferMessage.size();
      eof = inputBufferMessage.eof();
    }

    if (size) {
      dataSource->setSize(size);
      dataSink->setSize(size);

      runner.run(eof);
      output_size = size; // Might differ in the future
    }

    // Send a processing response for the input agent (eof, perfmon)
    if (firstAgentIt->second->input_buffer) {
      ServerProcessedBuffer msg;
      msg.set_eof(eof);
//      msg.set_message(); // Profiling Results

      // If there's only one agent in total, make sure to tell him the output size if necessary
      if (firstAgentIt == lastAgentIt && lastAgentIt->second->output_buffer)
        msg.set_size(output_size);

      firstAgentIt->second->socket.send_message<message_type::SERVER_PROCESSED_BUFFER>(msg);
    }

    // Send a processing response for the output agent (eof, perfmon, size)
    if (firstAgentIt != lastAgentIt && lastAgentIt->second->output_buffer) {
      ServerProcessedBuffer msg;
      msg.set_eof(eof);
//      msg.set_message(); // Profiling Results
      msg.set_size(output_size);
      lastAgentIt->second->socket.send_message<message_type::SERVER_PROCESSED_BUFFER>(msg);
    }

    if (eof) {
      // Send a processing response to all other agents (eof, perfmon)
    for (auto operatorAgentPair = std::next(firstAgentIt, 1); operatorAgentPair != std::next(lastAgentIt, 1); ++operatorAgentPair) {
        ServerProcessedBuffer msg;
        msg.set_eof(true);
//      msg.set_message(); // Profiling Results
        operatorAgentPair->second->socket.send_message<message_type::SERVER_PROCESSED_BUFFER>(msg);
      }
      return;
    }

    if (lastAgentIt->second->output_buffer) {
      // Wait for output client push buffer message (i.e. 'ready')
      if (firstAgentIt == lastAgentIt) {
        inputBufferMessage = firstAgentIt->second->socket.receive_message<message_type::AGENT_PUSH_BUFFER>();
      } else {
        lastAgentIt->second->socket.receive_message<message_type::AGENT_PUSH_BUFFER>();
      }
    }

    if (firstAgentIt != lastAgentIt && firstAgentIt->second->input_buffer) {
      // Wait for input client push buffer message (i.e. the next inputBufferMessage)
      inputBufferMessage = firstAgentIt->second->socket.receive_message<message_type::AGENT_PUSH_BUFFER>();
    }
  }
}

} // namespace metal
