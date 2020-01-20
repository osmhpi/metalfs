#include "pipeline_builder.hpp"

#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <utility>

#include <cxxopts.hpp>

#include <metal-driver-messages/message_header.hpp>
#include <metal-driver-messages/messages.hpp>
#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/operator_specification.hpp>

#include "pseudo_operators.hpp"

namespace metal {

PipelineBuilder::PipelineBuilder(
    std::shared_ptr<metal::OperatorRegistry> registry,
    std::vector<std::shared_ptr<OperatorAgent>> pipeline_agents)
    : _registry(std::move(registry)),
      _pipeline_agents(std::move(pipeline_agents)) {
  for (const auto &op : _registry->operatorSpecifications()) {
    // TODO: Don't do this every time a pipeline starts
    _operatorOptions.insert(
        std::make_pair(op.first, buildOperatorOptions(*op.second)));
  }
}

cxxopts::Options PipelineBuilder::buildOperatorOptions(
    const OperatorSpecification &op) {
  auto options = cxxopts::Options(op.id(), op.description());

  options.add_option("", "h", "help", "Print help",
                     cxxopts::value<bool>()->default_value("false"), "");
  options.add_option("", "p", "profile", "Enable profiling",
                     cxxopts::value<bool>()->default_value("false"), "");

  for (const auto &keyOptionPair : op.optionDefinitions()) {
    const auto &option = keyOptionPair.second;
    switch (option.type()) {
      case OptionType::Bool: {
        options.add_option("", option.shrt(), option.key(),
                           option.description(),
                           cxxopts::value<bool>()->default_value("false"), "");
        break;
      }
      case OptionType::Uint: {
        options.add_option("", option.shrt(), option.key(),
                           option.description(), cxxopts::value<uint32_t>(),
                           "");
        break;
      }
      case OptionType::Buffer: {
        options.add_option("", option.shrt(), option.key(),
                           option.description(), cxxopts::value<std::string>(),
                           "");
        break;
      }
    }
  }

  return options;
}

std::vector<std::pair<std::shared_ptr<const OperatorSpecification>,
                      std::shared_ptr<OperatorAgent>>>
PipelineBuilder::resolveOperatorSpecifications() {
  // Check if each requested operator is used only once and look up the
  // operator specification

  std::vector<std::pair<std::shared_ptr<const OperatorSpecification>,
                        std::shared_ptr<OperatorAgent>>>
      result;

  std::unordered_set<std::shared_ptr<const OperatorSpecification>>
      operatorsWithAgents;

  for (const auto &agent : _pipeline_agents) {
    if (DatagenOperator::isDatagenAgent(*agent)) {
      if (result.size() > 0)
        throw std::runtime_error(
            "Operator can only be used as the start of an FPGA pipeline");

      result.emplace_back(nullptr, agent);
      continue;
    }

    auto op = _registry->operatorSpecifications().find(agent->operatorType());
    if (op == _registry->operatorSpecifications().end()) {
      throw std::runtime_error("Unknown operator was requested");
    }

    if (!operatorsWithAgents.emplace(op->second).second) {
      throw std::runtime_error("Operator was used more than once");
    }

    result.emplace_back(std::make_pair(op->second, agent));
  }

  return result;
}

ConfiguredPipeline PipelineBuilder::configure() {
  auto orderedOperatorSpecsAndAgents = resolveOperatorSpecifications();

  ConfiguredPipeline result;
  result.operatorAgents = _pipeline_agents;

  {
    // Build the Pipeline
    std::vector<OperatorContext> operatorContexts;
    for (auto &operatorAgentPair : orderedOperatorSpecsAndAgents) {
      if (operatorAgentPair.first != nullptr) {
        operatorContexts.emplace_back(instantiateOperator(
            operatorAgentPair.first, operatorAgentPair.second));
      }
    }

    result.pipeline =
        std::make_shared<Pipeline>(std::move(operatorContexts));
  }

  // Establish pipeline data source and sink
  result.dataSourceAgent = orderedOperatorSpecsAndAgents.front().second;
  if (result.dataSourceAgent->internalInputFile().empty() &&
      !DatagenOperator::isDatagenAgent(*result.dataSourceAgent)) {
    result.dataSourceAgent->createInputBuffer();
  }

  result.dataSinkAgent = orderedOperatorSpecsAndAgents.back().second;
  if (result.dataSinkAgent->internalOutputFile().empty()) {
    result.dataSinkAgent->createOutputBuffer();
  }

  // Validate datagen (if necessary)
  if (DatagenOperator::isDatagenAgent(*result.dataSourceAgent)) {
    DatagenOperator::validate(*result.dataSourceAgent);
  }

  // Tell agents that they're accepted
  for (auto &operatorAgentPair : orderedOperatorSpecsAndAgents) {
    RegistrationResponse response;

    response.set_valid(true);

    if (operatorAgentPair.second->inputBuffer() != std::nullopt)
      response.set_input_buffer_filename(
          operatorAgentPair.second->inputBuffer()->filename());
    if (operatorAgentPair.second->outputBuffer() != std::nullopt)
      response.set_output_buffer_filename(
          operatorAgentPair.second->outputBuffer()->filename());

    operatorAgentPair.second->sendRegistrationResponse(response);
  }

  return result;
}

OperatorContext PipelineBuilder::instantiateOperator(
    std::shared_ptr<const OperatorSpecification> op,
    std::shared_ptr<OperatorAgent> agent) {
  auto &options = _operatorOptions.at(op->id());
  auto parseResult = agent->parseOptions(options);

  Operator userOperator(op);

  for (const auto &optionType : op->optionDefinitions()) {
    auto result = parseResult[optionType.first];

    switch (optionType.second.type()) {
      case OptionType::Uint: {
        userOperator.setOption(optionType.first, result.as<uint32_t>());
        break;
      }
      case OptionType::Bool: {
        userOperator.setOption(optionType.first, result.as<bool>());
        break;
      }
      case OptionType::Buffer: {
        if (!optionType.second.bufferSize().has_value())
          throw std::runtime_error("File option metadata not found");

        auto filePath = agent->resolvePath(result.as<std::string>());

        // TODO: Detect if path points to FPGA file

        auto buffer = std::make_unique<std::vector<char>>(
            optionType.second.bufferSize().value());

        auto fp = fopen(filePath.c_str(), "r");
        if (fp == nullptr) {
          throw std::runtime_error("Could not open file");
        }

        auto nread = fread(buffer->data(), 1, buffer->size(), fp);
        if (nread <= 0) {
          fclose(fp);
          throw std::runtime_error("Could not read from file");
        }

        fclose(fp);

        userOperator.setOption(optionType.first, std::move(buffer));
      }
    }
  }

  OperatorContext runtimeContext(std::move(userOperator));
  runtimeContext.set_profiling_enabled(parseResult["profile"].as<bool>());
  return runtimeContext;
}

}  // namespace metal
