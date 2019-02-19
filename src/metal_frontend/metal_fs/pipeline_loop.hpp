#pragma once

namespace metal {
class PipelineLoop {
 public:
  explicit PipelineLoop(std::shared_ptr<PipelineDefinition> pipeline);

  void run();

 protected:
  std::shared_ptr<PipelineDefinition> _pipeline;
};

} // namespace metal
