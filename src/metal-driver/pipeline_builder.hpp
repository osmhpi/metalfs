#pragma once

#include <memory>
#include <unordered_set>

#include <cxxopts.hpp>

#include <metal-pipeline/operator_registry.hpp>
#include <metal-pipeline/operator_context.hpp>

#include "client_error.hpp"
#include "configured_pipeline.hpp"
#include "operator_agent.hpp"

namespace metal {

class PipelineBuilder {
 public:
  explicit PipelineBuilder(
      std::shared_ptr<metal::OperatorRegistry> registry,
      std::vector<std::shared_ptr<OperatorAgent>> pipeline_agents);

  ConfiguredPipeline configure();

 protected:
  static cxxopts::Options buildOperatorOptions(
      const OperatorSpecification& spec);

  std::vector<std::pair<std::shared_ptr<const OperatorSpecification>,
                        std::shared_ptr<OperatorAgent>>>
  resolveOperatorSpecifications();

  OperatorContext instantiateOperator(
      std::shared_ptr<const OperatorSpecification> op,
      std::shared_ptr<OperatorAgent> agent);

  std::shared_ptr<metal::OperatorRegistry> _registry;
  std::vector<std::shared_ptr<OperatorAgent>> _pipeline_agents;
  std::unordered_map<std::string, cxxopts::Options> _operatorOptions;
};

}  // namespace metal
