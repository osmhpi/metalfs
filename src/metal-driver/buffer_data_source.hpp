#pragma once

#include <memory>

#include <metal-filesystem-pipeline/file_data_source.hpp>

namespace metal {

class FileSourceRuntimeContext;
class PipelineDefinition;
class RegisteredAgent;

class BufferSourceRuntimeContext : public FileSourceRuntimeContext {
 public:
  explicit BufferSourceRuntimeContext(
      std::shared_ptr<RegisteredAgent> agent,
      std::shared_ptr<PipelineDefinition> pipeline,
      bool skipSendingProcessingResponse);

  const DataSource dataSource() const final;
  void configure(SnapAction &action, bool initial) final;
  void finalize(SnapAction &action) final;
  uint64_t reportTotalSize();

  bool endOfInput() const final;

 protected:
  std::shared_ptr<RegisteredAgent> _agent;
  std::shared_ptr<PipelineDefinition> _pipeline;
  bool _skipSendingProcessingResponse;
  uint64_t _remainingTotalSize;
  uint64_t _size;
  bool _eof;
};

}  // namespace metal
