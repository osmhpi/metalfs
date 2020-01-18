#include "buffer_data_source.hpp"

#include <spdlog/spdlog.h>

#include <metal-pipeline/pipeline_definition.hpp>

#include "pseudo_operators.hpp"
#include "registered_agent.hpp"

namespace metal {

BufferSourceRuntimeContext::BufferSourceRuntimeContext(
    std::shared_ptr<RegisteredAgent> agent,
    std::shared_ptr<PipelineDefinition> pipeline,
    bool skipSendingProcessingResponse)
    : FileSourceRuntimeContext(fpga::AddressType::NVMe, fpga::MapType::NVMe, "",
                               0, 0),
      _agent(agent),
      _pipeline(pipeline),
      _skipSendingProcessingResponse(skipSendingProcessingResponse) {
  if (_agent->input_buffer) {
    // Nothing to do
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _remainingTotalSize = DatagenOperator::datagenLength(*_agent);
  } else if (!agent->internal_input_file.empty()) {
    _filename = agent->internal_input_file;
    loadExtents();
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
  if (initial || _agent->input_buffer) {
    request = _agent->receiveProcessingRequest();
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

  if ((endOfInput() || _agent->input_buffer) &&
      !_skipSendingProcessingResponse) {
    ProcessingResponse msg;
    msg.set_eof(endOfInput());
    msg.set_message(_profilingResults);
    _agent->sendProcessingResponse(msg);
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
