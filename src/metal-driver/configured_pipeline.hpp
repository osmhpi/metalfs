#pragma once

#include <memory>
#include <unordered_map>

namespace metal {

class RegisteredAgent;
class PipelineDefinition;

struct ConfiguredPipeline {
  std::shared_ptr<PipelineDefinition> pipeline;

  std::shared_ptr<RegisteredAgent> dataSourceAgent;
  std::shared_ptr<RegisteredAgent> dataSinkAgent;

  std::vector<std::shared_ptr<RegisteredAgent>> operatorAgents;
};

}  // namespace metal
