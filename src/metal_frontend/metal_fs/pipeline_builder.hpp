#pragma once

#include <memory>
#include <unordered_set>
#include <metal_pipeline/operator_registry.hpp>
#include "registered_agent.hpp"
#include "../../../third_party/cxxopts/include/cxxopts.hpp"

namespace metal {

class PipelineDefinition;

class PipelineBuilder {
 public:
  explicit PipelineBuilder(std::vector<std::shared_ptr<RegisteredAgent>> pipeline_agents);

  std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> configure();

 protected:
  cxxopts::Options buildOperatorOptions(std::shared_ptr<UserOperator> op);
  std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> resolve_operators();
  void set_operator_options_from_agent_request(
          std::shared_ptr<AbstractOperator> op,
          std::shared_ptr<RegisteredAgent> agent);
  std::string resolvePath(std::string relative_or_absolue_path, std::string working_dir);

  OperatorRegistry _registry;
  std::vector<std::shared_ptr<RegisteredAgent>> _pipeline_agents;
  std::unordered_map<std::string, cxxopts::Options> _operatorOptions;
};

} // namespace metal