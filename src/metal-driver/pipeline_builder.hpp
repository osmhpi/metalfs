#pragma once

#include <cxxopts.hpp>
#include <memory>
#include <metal-pipeline/operator_registry.hpp>
#include <unordered_set>
#include "registered_agent.hpp"

namespace metal {

class PipelineDefinition;

class ClientError : public std::runtime_error {
 public:
  explicit ClientError(std::shared_ptr<RegisteredAgent> agent,
                       const std::string &message)
      : std::runtime_error(message), _agent(agent) {}
  std::shared_ptr<RegisteredAgent> agent() const { return _agent; }

 protected:
  std::shared_ptr<RegisteredAgent> _agent;
};

class PipelineBuilder {
 public:
  explicit PipelineBuilder(
      std::shared_ptr<metal::OperatorRegistry> registry,
      std::vector<std::shared_ptr<RegisteredAgent>> pipeline_agents);

  std::vector<std::pair<std::shared_ptr<AbstractOperator>,
                        std::shared_ptr<RegisteredAgent>>>
  configure();

 protected:
  static cxxopts::Options buildOperatorOptions(
      const std::shared_ptr<AbstractOperator> &op);
  std::vector<std::pair<std::shared_ptr<AbstractOperator>,
                        std::shared_ptr<RegisteredAgent>>>
  resolve_operators();
  void set_operator_options_from_agent_request(
      std::shared_ptr<AbstractOperator> op,
      std::shared_ptr<RegisteredAgent> agent);
  std::string resolvePath(std::string relative_or_absolute_path,
                          std::string working_dir);

  std::shared_ptr<metal::OperatorRegistry> _registry;
  std::vector<std::shared_ptr<RegisteredAgent>> _pipeline_agents;
  std::unordered_map<std::string, cxxopts::Options> _operatorOptions;
};

}  // namespace metal
