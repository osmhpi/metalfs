#pragma once

#include <memory>
#include <vector>

#include <metal-pipeline/fpga_action_factory.hpp>

#include "configured_pipeline.hpp"

namespace metal {

class Operator;
class OperatorAgent;

class PipelineLoop {
 public:
  PipelineLoop(ConfiguredPipeline pipeline, std::shared_ptr<FpgaActionFactory> actionFactory)
      : _pipeline(std::move(pipeline)), _actionFactory(actionFactory) {}

  void run();

 protected:
  std::shared_ptr<OperatorAgent> _dataSourceAgent;
  std::shared_ptr<OperatorAgent> _dataSinkAgent;
  ConfiguredPipeline _pipeline;
  std::shared_ptr<FpgaActionFactory> _actionFactory;
};

}  // namespace metal
