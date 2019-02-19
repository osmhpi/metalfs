#include <metal_filesystem_pipeline/filesystem_pipeline_runner.hpp>
#include <Messages.pb.h>
#include <metal_frontend/messages/message_header.hpp>
#include "pipeline_loop.hpp"

namespace metal {

void PipelineLoop::run() {
  FilesystemPipelineRunner runner(_pipeline);

  // Wait until all clients signal ready
  ClientPushBuffer inputBufferMessage;
  for (auto &operatorAgentPair : operators) {
    auto req = MessageHeader::receive(operatorAgentPair.second->socket);
    if (req.type() != message_type::AGENT_PUSH_BUFFER)
      throw std::runtime_error("Unexpected message");

    auto msg = receive_message<ClientPushBuffer>(operatorAgentPair.second->socket, req);
  }

  for (;;) {
    ServerProcessedBuffer processing_response;

    size_t size = 0;
    bool eof = false;

    size_t output_size = 0;

    // If the client provides us with input data, forward it to the pipeline
    if (operators.front().second->input_buffer) {
      size = inputBufferMessage.size();
      eof = inputBufferMessage.eof();
    }

    if (size) {
      runner.run(eof);
    }

    // Send a processing response for the input agent (eof, perfmon)

    // Send a processing response for the output agent (eof, perfmon, size)


    if (eof) {
      // Send a processing response to all other agents (eof, perfmon)
    }

    if (operators.back().second->output_buffer) {
      // Wait for output client push buffer message (i.e. 'ready')
    }

    if (operators.front().second->input_buffer) {
      // Wait for output client push buffer message (i.e. the next inputBufferMessage)
    }
  }

  for (const auto &a : _pipeline_agents) {
    close(a->socket);
  }

  _pipeline_agents.clear();
}

} // namespace metal
