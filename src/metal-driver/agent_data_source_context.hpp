#pragma once

#include <memory>

#include <metal-filesystem-pipeline/file_data_source_context.hpp>

namespace metal {

class FileDataSourceContext;
class FilesystemContext;
class OperatorAgent;
class Pipeline;

class AgentDataSourceContext : public FileDataSourceContext {
 public:
  explicit AgentDataSourceContext(std::shared_ptr<FilesystemContext> filesystem,
                                  std::shared_ptr<OperatorAgent> agent,
                                  std::shared_ptr<Pipeline> pipeline,
                                  bool skipSendingProcessingResponse);

  const DataSource dataSource() const final;
  void configure(SnapAction &action, bool initial) final;
  void finalize(SnapAction &action) final;
  uint64_t reportTotalSize();

  bool endOfInput() const final;

 protected:
  std::shared_ptr<OperatorAgent> _agent;
  std::shared_ptr<Pipeline> _pipeline;
  bool _skipSendingProcessingResponse;
  uint64_t _remainingTotalSize;
  uint64_t _size;
  bool _eof;
};

}  // namespace metal
