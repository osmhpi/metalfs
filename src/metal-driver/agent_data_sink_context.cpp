#include "agent_data_sink_context.hpp"

#include <spdlog/spdlog.h>

#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>
#include <metal-pipeline/pipeline.hpp>

#include "filesystem_fuse_handler.hpp"
#include "operator_agent.hpp"
#include "pseudo_operators.hpp"

namespace metal {

AgentDataSinkContext::AgentDataSinkContext(std::shared_ptr<OperatorAgent> agent,
                                           std::shared_ptr<Pipeline> pipeline,
                                           bool skipReceivingProcessingRequest)
    : FileDataSinkContext(agent->internalOutputFile().second,
                          agent->internalOutputFile().first, 0, BufferSize),
      _agent(agent),
      _pipeline(pipeline),
      _skipReceivingProcessingRequest(skipReceivingProcessingRequest) {}

const DataSink AgentDataSinkContext::dataSink() const {
  // The data sink can be of three types: Agent-Buffer, Null or File

  if (_agent->outputBuffer()) {
    return DataSink(_agent->outputBuffer()->current(),
                    _agent->outputBuffer()->size());
  } else if (DevNullFile::isNullOutput(*_agent)) {
    return DataSink(0, 0, fpga::AddressType::Null);
  } else if (_inode_id != 0) {
    return FileDataSinkContext::dataSink();
  } else {
    throw std::runtime_error("Unknown data sink");
  }
}

void AgentDataSinkContext::configure(SnapAction &action, uint64_t inputSize,
                                     bool initial) {
  if ((initial || _agent->outputBuffer()) && !_skipReceivingProcessingRequest) {
    _agent->receiveProcessingRequest();
  }

  if (_inode_id != 0) {
    return FileDataSinkContext::configure(action, inputSize, initial);
  }
}

void AgentDataSinkContext::finalize(SnapAction &action, uint64_t outputSize,
                                    bool endOfInput) {
  ProcessingResponse msg{};
  msg.set_eof(endOfInput);

  if (_pipeline->operators().size()) {
    msg.set_message(_pipeline->operators().back().profilingResults());
  } else {
    // As there is no pseudo-operator for data sinks, this should only be set
    // in exceptional cases (e.g. pipeline consisting only of `datagen -p`).
    msg.set_message(_profilingResults);
  }

  if (_agent->outputBuffer()) {
    _agent->outputBuffer()->swap();
    msg.set_size(outputSize);
  } else if (DevNullFile::isNullOutput(*_agent)) {
    // Nothing to do
  } else if (_inode_id != 0) {
    FileDataSinkContext::finalize(action, outputSize, endOfInput);
  }

  if (endOfInput || _agent->outputBuffer() || _agent->inputBuffer()) {
    _agent->sendProcessingResponse(msg);
    if (endOfInput) {
      _agent->setTerminated();
    }
  }
}

void AgentDataSinkContext::prepareForTotalSize(uint64_t totalSize) {
  if (_inode_id != 0) {
    FileDataSinkContext::prepareForTotalSize(totalSize);
  }
}

}  // namespace metal
