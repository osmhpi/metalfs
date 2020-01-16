#pragma once

#include <memory>

#include <metal-filesystem-pipeline/file_data_sink.hpp>

namespace metal {

class RegisteredAgent;
class FileSinkRuntimeContext;

class BufferSinkRuntimeContext : public FileSinkRuntimeContext {
 public:
  explicit BufferSinkRuntimeContext(std::shared_ptr<RegisteredAgent> agent,
                                    bool skipReceivingProcessingRequest);

  const DataSink dataSink() const final;
  void configure(SnapAction &action, bool initial) final;
  void finalize(SnapAction &action, uint64_t outputSize, bool endOfInput) final;
  void prepareForTotalSize(uint64_t totalSize);

 protected:
  std::shared_ptr<RegisteredAgent> _agent;
  bool _skipReceivingProcessingRequest;
  uint64_t _size;
};

}  // namespace metal
