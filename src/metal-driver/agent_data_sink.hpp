#pragma once

#include <memory>

#include <metal-filesystem-pipeline/file_data_sink.hpp>

namespace metal {

class FileSinkRuntimeContext;
class PipelineDefinition;
class RegisteredAgent;

class BufferSinkRuntimeContext : public FileSinkRuntimeContext {
 public:
  explicit BufferSinkRuntimeContext(
      std::shared_ptr<RegisteredAgent> agent,
      std::shared_ptr<PipelineDefinition> pipeline,
      bool skipReceivingProcessingRequest);

  const DataSink dataSink() const final;
  void configure(SnapAction &action, bool initial) final;
  void finalize(SnapAction &action, uint64_t outputSize, bool endOfInput) final;
  void prepareForTotalSize(uint64_t totalSize);

 protected:
  std::shared_ptr<RegisteredAgent> _agent;
  std::shared_ptr<PipelineDefinition> _pipeline;
  bool _skipReceivingProcessingRequest;
  uint64_t _size;
};

}  // namespace metal
