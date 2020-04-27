#pragma once

#include <memory>
#include <vector>

#include <metal-pipeline/card.hpp>

#include "configured_pipeline.hpp"

namespace metal {

class Operator;
class OperatorAgent;

class PipelineLoop {
 public:
  PipelineLoop(ConfiguredPipeline pipeline, Card card)
      : _pipeline(std::move(pipeline)), _card(card) {}

  void run();

 protected:
  std::shared_ptr<OperatorAgent> _dataSourceAgent;
  std::shared_ptr<OperatorAgent> _dataSinkAgent;
  ConfiguredPipeline _pipeline;
  Card _card;
};

}  // namespace metal
