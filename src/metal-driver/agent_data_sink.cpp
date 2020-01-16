#include "agent_data_sink.hpp"

#include <spdlog/spdlog.h>

#include "pseudo_operators.hpp"
#include "registered_agent.hpp"

namespace metal {

BufferSinkRuntimeContext::BufferSinkRuntimeContext(
    std::shared_ptr<RegisteredAgent> agent, bool skipReceivingProcessingRequest)
    : FileSinkRuntimeContext(fpga::AddressType::NVMe, fpga::MapType::NVMe, "",
                             0, 0),
      _agent(agent),
      _skipReceivingProcessingRequest(skipReceivingProcessingRequest) {
  _agent->socket.receiveMessage<message_type::ProcessingRequest>();
}

const DataSink BufferSinkRuntimeContext::dataSink() const {
  // The data sink can be of three types: Agent-Buffer, Null or File

  if (_agent->output_buffer) {
    return DataSink(_agent->output_buffer->current(),
                    _agent->output_buffer->size());
  } else if (DevNullFile::isNullOutput(*_agent)) {
    return DataSink(0, 0, fpga::AddressType::Null);
  } else if (!_filename.empty()) {
    return FileSinkRuntimeContext::dataSink();
  } else {
    throw std::runtime_error("Unknown data sink");
  }
}

void BufferSinkRuntimeContext::configure(SnapAction &action, bool initial) {
  if ((!initial || _agent->output_buffer) && !_skipReceivingProcessingRequest) {
    _agent->socket.receiveMessage<message_type::ProcessingRequest>();
  }

  if (!_filename.empty()) {
    return FileSinkRuntimeContext::configure(action, initial);
  }
}

void BufferSinkRuntimeContext::finalize(SnapAction &action, uint64_t outputSize,
                                        bool endOfInput) {
  ProcessingResponse msg;
  msg.set_eof(endOfInput);

  if (_agent->output_buffer) {
    _agent->output_buffer.value().swap();
    msg.set_size(outputSize);
  } else if (DevNullFile::isNullOutput(*_agent)) {
    // Nothing to do
  } else if (!_filename.empty()) {
    FileSinkRuntimeContext::finalize(action, outputSize, endOfInput);
  }

  if (endOfInput || _agent->input_buffer) {
    _agent->socket.send_message<message_type::ProcessingResponse>(msg);
    spdlog::trace("ProcessingResponse({}, {})", msg.size(), msg.eof());
  }
}

void BufferSinkRuntimeContext::prepareForTotalSize(uint64_t totalSize) {
  if (!_filename.empty()) {
    FileSinkRuntimeContext::prepareForTotalSize(totalSize);
  }
}

}  // namespace metal
