#pragma once

#include <memory>
#include <unordered_set>

#include <cxxopts.hpp>

#include <metal-pipeline/operator_registry.hpp>
#include <metal-pipeline/operator_runtime_context.hpp>

#include "client_error.hpp"
#include "configured_pipeline.hpp"
#include "registered_agent.hpp"

namespace metal {

class PipelineBuilder {
 public:
  explicit PipelineBuilder(
      std::shared_ptr<metal::OperatorRegistry> registry,
      std::vector<std::shared_ptr<RegisteredAgent>> pipeline_agents);

  ConfiguredPipeline configure();

 protected:
  static cxxopts::Options buildOperatorOptions(
      const UserOperatorSpecification& spec);

  std::vector<std::pair<std::shared_ptr<const UserOperatorSpecification>,
                        std::shared_ptr<RegisteredAgent>>>
  resolveOperatorSpecifications();

  UserOperatorRuntimeContext instantiateOperator(
      std::shared_ptr<const UserOperatorSpecification> op,
      std::shared_ptr<RegisteredAgent> agent);

  std::shared_ptr<metal::OperatorRegistry> _registry;
  std::vector<std::shared_ptr<RegisteredAgent>> _pipeline_agents;
  std::unordered_map<std::string, cxxopts::Options> _operatorOptions;
};

}  // namespace metal
