#pragma once

#include <memory>

#include <metal-filesystem-pipeline/file_data_source.hpp>

namespace metal {

class RegisteredAgent;
class FileSourceRuntimeContext;

class BufferSourceRuntimeContext : public FileSourceRuntimeContext {
 public:
  explicit BufferSourceRuntimeContext(std::shared_ptr<RegisteredAgent> agent,
                                      bool skipSendingProcessingResponse);

  const DataSource dataSource() const final;
  void configure(SnapAction &action, bool initial) final;
  void finalize(SnapAction &action) final;
  uint64_t reportTotalSize();

  bool endOfInput() const final;

 protected:
  std::shared_ptr<RegisteredAgent> _agent;
  bool _skipSendingProcessingResponse;
  uint64_t _remainingTotalSize;
  uint64_t _size;
  bool _eof;
};

}  // namespace metal
