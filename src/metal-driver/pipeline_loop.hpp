#pragma once

#include <memory>
#include <vector>

#include "configured_pipeline.hpp"

namespace metal {

class UserOperator;
class RegisteredAgent;

class PipelineLoop {

 public:
  PipelineLoop(ConfiguredPipeline pipeline, int card)
      : _pipeline(std::move(pipeline)), _card(card) {}

  void run();

 protected:
  std::shared_ptr<RegisteredAgent> _dataSourceAgent;
  std::shared_ptr<RegisteredAgent> _dataSinkAgent;
  ConfiguredPipeline _pipeline;
  int _card;
};

}  // namespace metal
