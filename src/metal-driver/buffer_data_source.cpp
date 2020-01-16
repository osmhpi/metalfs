#include "buffer_data_source.hpp"

#include <spdlog/spdlog.h>

#include <metal-filesystem-pipeline/file_data_source.hpp>

#include "pseudo_operators.hpp"
#include "registered_agent.hpp"

namespace metal {

BufferSourceRuntimeContext::BufferSourceRuntimeContext(
    std::shared_ptr<RegisteredAgent> agent, bool skipSendingProcessingResponse)
    : FileSourceRuntimeContext(fpga::AddressType::NVMe, fpga::MapType::NVMe, "",
                               0, 0),
      _agent(agent),
      _skipSendingProcessingResponse(skipSendingProcessingResponse) {
  if (_agent->input_buffer) {
    //
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _remainingTotalSize = DatagenOperator::datagenLength(*_agent);
  } else if (!agent->internal_input_file.empty()) {
    _filename = agent->internal_input_file;
  } else {
    throw std::runtime_error("Unknown data source");
  }
}

const DataSource BufferSourceRuntimeContext::dataSource() const {
  // The data source can be of three types: Agent-Buffer, Random or File

  if (_agent->input_buffer) {
    return DataSource(_agent->input_buffer->current(), _size);
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    return DataSource(0, _size, fpga::AddressType::Random);
  } else if (!_filename.empty()) {
    return FileSourceRuntimeContext::dataSource();
  } else {
    throw std::runtime_error("Unknown data source");
  }
}

void BufferSourceRuntimeContext::configure(SnapAction &action, bool initial) {
  ProcessingRequest request;
  if (!initial || _agent->output_buffer) {
    request = _agent->socket.receiveMessage<message_type::ProcessingRequest>();
    spdlog::trace("ProcessingRequest({}, {})", request.size(), request.eof());
  }

  if (_agent->input_buffer) {
    _size = request.size();
    _eof = request.eof();
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _size = std::min(_remainingTotalSize, BufferSize);
    _eof = _remainingTotalSize == _size;
  } else if (!_filename.empty()) {
    return FileSourceRuntimeContext::configure(action, initial);
  }
}

void BufferSourceRuntimeContext::finalize(SnapAction &action) {
  if (_agent->input_buffer) {
    _agent->input_buffer.value().swap();
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _remainingTotalSize -= _size;
  } else if (!_filename.empty()) {
    FileSourceRuntimeContext::finalize(action);
  }

  if ((endOfInput() || _agent->input_buffer) && !_skipSendingProcessingResponse) {
      ProcessingResponse msg;
      msg.set_eof(endOfInput());
      msg.set_message(_profilingResults);
      _agent->socket.send_message<message_type::ProcessingResponse>(msg);
      spdlog::trace("ProcessingResponse({}, {})", msg.size(), msg.eof());
  }
}

uint64_t BufferSourceRuntimeContext::reportTotalSize() {
  if (DatagenOperator::isDatagenAgent(*_agent)) {
    return _remainingTotalSize;
  } else if (!_filename.empty()) {
    return FileSourceRuntimeContext::reportTotalSize();
  }
  return 0;
}

bool BufferSourceRuntimeContext::endOfInput() const {
  if (!_filename.empty()) {
    return FileSourceRuntimeContext::endOfInput();
  }
  return _eof;
}

}  // namespace metal
