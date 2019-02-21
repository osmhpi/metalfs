#include <utility>

#pragma once

namespace metal {

class RegisteredAgent;

class PipelineLoop {
 public:
  explicit PipelineLoop(std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> pipeline)
    : _pipeline(std::move(pipeline)) {}

  void run();

 protected:
  std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> _pipeline;
};

} // namespace metal
