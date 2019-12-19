#include <utility>

#pragma once

namespace metal {

class RegisteredAgent;

class PipelineLoop {
 public:
  PipelineLoop(std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> pipeline, int card)
    : _pipeline(std::move(pipeline)), _card(card) {}

  void run();

 protected:
  std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> _pipeline;
  int _card;
};

} // namespace metal
