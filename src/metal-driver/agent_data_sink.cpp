#include "agent_data_sink.hpp"

#include <spdlog/spdlog.h>

#include <metal-pipeline/pipeline_definition.hpp>

#include "pseudo_operators.hpp"
#include "registered_agent.hpp"

namespace metal {

BufferSinkRuntimeContext::BufferSinkRuntimeContext(
    std::shared_ptr<RegisteredAgent> agent,
    std::shared_ptr<PipelineDefinition> pipeline,
    bool skipReceivingProcessingRequest)
    : FileSinkRuntimeContext(fpga::AddressType::NVMe, fpga::MapType::NVMe, "",
                             0, 0),
      _agent(agent),
      _pipeline(pipeline),
      _skipReceivingProcessingRequest(skipReceivingProcessingRequest) {}

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
  if ((initial || _agent->output_buffer) && !_skipReceivingProcessingRequest) {
    _agent->receiveProcessingRequest();
  }

  if (!_filename.empty()) {
    return FileSinkRuntimeContext::configure(action, initial);
  }
}

void BufferSinkRuntimeContext::finalize(SnapAction &action, uint64_t outputSize,
                                        bool endOfInput) {
  ProcessingResponse msg;
  msg.set_eof(endOfInput);
  msg.set_message(_profilingResults);

  if (_agent->output_buffer) {
    _agent->output_buffer.value().swap();
    msg.set_size(outputSize);
    if (_pipeline->operators().size()) {
      msg.set_message(_pipeline->operators().back().profilingResults());
    }
  } else if (DevNullFile::isNullOutput(*_agent)) {
    // Nothing to do
  } else if (!_filename.empty()) {
    FileSinkRuntimeContext::finalize(action, outputSize, endOfInput);
  }

  if (endOfInput || _agent->output_buffer) {
    _agent->sendProcessingResponse(msg);
  }
}

void BufferSinkRuntimeContext::prepareForTotalSize(uint64_t totalSize) {
  if (!_filename.empty()) {
    FileSinkRuntimeContext::prepareForTotalSize(totalSize);
  }
}

}  // namespace metal
