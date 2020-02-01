#pragma once

#include <memory>
#include <unordered_map>

namespace metal {

class OperatorAgent;
class Pipeline;

struct ConfiguredPipeline {
  std::shared_ptr<Pipeline> pipeline;

  std::shared_ptr<OperatorAgent> dataSourceAgent;
  std::shared_ptr<OperatorAgent> dataSinkAgent;

  std::vector<std::shared_ptr<OperatorAgent>> operatorAgents;
};

}  // namespace metal
