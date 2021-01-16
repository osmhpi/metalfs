#include "agent_data_source_context.hpp"

#include <spdlog/spdlog.h>

#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>
#include <metal-pipeline/pipeline.hpp>

#include "filesystem_fuse_handler.hpp"
#include "operator_agent.hpp"
#include "pseudo_operators.hpp"

namespace metal {

AgentDataSourceContext::AgentDataSourceContext(
    std::shared_ptr<OperatorAgent> agent, std::shared_ptr<Pipeline> pipeline,
    bool skipSendingProcessingResponse)
    : FileDataSourceContext(agent->internalInputFile().second, agent->internalInputFile().first, 0, BufferSize),
      _agent(agent),
      _pipeline(pipeline),
      _skipSendingProcessingResponse(skipSendingProcessingResponse) {
  if (_agent->inputBuffer()) {
    // Nothing to do
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _remainingTotalSize = DatagenOperator::datagenLength(*_agent);
  } else if (agent->internalInputFile().first != 0) {
    // Nothing to do
  } else {
    throw std::runtime_error("Unknown data source");
  }
}

const DataSource AgentDataSourceContext::dataSource() const {
  // The data source can be of three types: Agent-Buffer, Random or File

  if (_agent->inputBuffer()) {
    return DataSource(_agent->inputBuffer()->current(), _size);
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    return DataSource(0, _size, fpga::AddressType::Random);
  } else if (_inode_id != 0) {
    return FileDataSourceContext::dataSource();
  } else {
    throw std::runtime_error("Unknown data source");
  }
}

void AgentDataSourceContext::configure(FpgaAction &action, bool initial) {
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
  } else if (_inode_id != 0) {
    return FileDataSourceContext::configure(action, initial);
  }
}

void AgentDataSourceContext::finalize(FpgaAction &action) {
  // Check for end-of-input *before* advancing any read offsets
  auto eof = endOfInput();

  if (_agent->inputBuffer()) {
    _agent->inputBuffer()->swap();
  } else if (DatagenOperator::isDatagenAgent(*_agent)) {
    _remainingTotalSize -= _size;
  } else if (_inode_id != 0) {
    FileDataSourceContext::finalize(action);
  }

  if ((eof || _agent->inputBuffer()) && !_skipSendingProcessingResponse) {
    ProcessingResponse msg;
    msg.set_eof(eof);

    if (DatagenOperator::isDatagenAgent(*_agent) ||
        MetalCatOperator::isMetalCatAgent(*_agent)) {
      msg.set_message(_profilingResults);
    } else if (_pipeline->operators().size()) {
      msg.set_message(_pipeline->operators().front().profilingResults());
    }

    _agent->sendProcessingResponse(msg);
  }
}

uint64_t AgentDataSourceContext::reportTotalSize() {
  if (DatagenOperator::isDatagenAgent(*_agent)) {
    return _remainingTotalSize;
  } else if (_inode_id != 0) {
    return FileDataSourceContext::reportTotalSize();
  }
  return 0;
}

bool AgentDataSourceContext::endOfInput() const {
  if (_inode_id != 0) {
    return FileDataSourceContext::endOfInput();
  }
  return _eof;
}

}  // namespace metal
