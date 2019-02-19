#pragma once

#include <memory>
#include <unordered_set>
#include <metal_pipeline/operator_registry.hpp>
#include "registered_agent.hpp"

namespace metal {

class PipelineDefinition;

class PipelineBuilder {
 public:
  explicit PipelineBuilder(std::unordered_set<std::shared_ptr<RegisteredAgent>> pipeline_agents);

  bool validate();
  std::shared_ptr<PipelineDefinition> configure();

 protected:
  cxxopts::Options buildOperatorOptions(std::shared_ptr<UserOperator> op);
  void set_operator_options_from_agent_request(
          std::shared_ptr<AbstractOperator> op,
          std::shared_ptr<RegisteredAgent> agent);
  std::string resolvePath(std::string relative_or_absolue_path, std::string working_dir);

  OperatorRegistry _registry;
  std::unordered_set<std::shared_ptr<RegisteredAgent>> _pipeline_agents;
  std::unordered_map<std::string, cxxopts::Options> _operatorOptions;
};

} // namespace metal
