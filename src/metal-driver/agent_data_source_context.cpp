#include "agent_data_source_context.hpp"

#include <spdlog/spdlog.h>

#include <metal-pipeline/pipeline.hpp>

#include "pseudo_operators.hpp"
#include "operator_agent.hpp"

namespace metal {

BufferSourceRuntimeContext::BufferSourceRuntimeContext(
    std::shared_ptr<OperatorAgent> agent,
    std::shared_ptr<Pipeline> pipeline,
    bool skipSendingProcessingResponse)
    : FileDataSourceContext(fpga::AddressType::NVMe, fpga::MapType::NVMe, "",
                               0, 0),
      _agent(agent),
      _pipeline(pipeline),
      _skipSendingProcessingResponse(skipSendingProcessingResponse) {
  if (_agent->inputBuffer()) {
    // Nothing to do
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _remainingTotalSize = DatagenOperator::datagenLength(*_agent);
  } else if (!agent->internalInputFile().empty()) {
    _filename = agent->internalInputFile();
    loadExtents();
  } else {
    throw std::runtime_error("Unknown data source");
  }
}

const DataSource BufferSourceRuntimeContext::dataSource() const {
  // The data source can be of three types: Agent-Buffer, Random or File

  if (_agent->inputBuffer()) {
    return DataSource(_agent->inputBuffer()->current(), _size);
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    return DataSource(0, _size, fpga::AddressType::Random);
  } else if (!_filename.empty()) {
    return FileDataSourceContext::dataSource();
  } else {
    throw std::runtime_error("Unknown data source");
  }
}

void BufferSourceRuntimeContext::configure(SnapAction &action, bool initial) {
  ProcessingRequest request;
  if (initial || _agent->inputBuffer()) {
    request = _agent->receiveProcessingRequest();
  }

  if (_agent->inputBuffer()) {
    _size = request.size();
    _eof = request.eof();
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _size = std::min(_remainingTotalSize, BufferSize);
    _eof = _remainingTotalSize == _size;
  } else if (!_filename.empty()) {
    return FileDataSourceContext::configure(action, initial);
  }
}

void BufferSourceRuntimeContext::finalize(SnapAction &action) {
  if (_agent->inputBuffer()) {
    _agent->inputBuffer()->swap();
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _remainingTotalSize -= _size;
  } else if (!_filename.empty()) {
    FileDataSourceContext::finalize(action);
  }

  if ((endOfInput() || _agent->inputBuffer()) &&
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
    return FileDataSourceContext::reportTotalSize();
  }
  return 0;
}

bool BufferSourceRuntimeContext::endOfInput() const {
  if (!_filename.empty()) {
    return FileDataSourceContext::endOfInput();
  }
  return _eof;
}

}  // namespace metal
