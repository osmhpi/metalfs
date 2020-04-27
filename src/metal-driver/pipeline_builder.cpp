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
#include <metal-pipeline/operator_specification.hpp>
#include <metal-pipeline/pipeline.hpp>

#include "filesystem_fuse_handler.hpp"
#include "metal_fuse_operations.hpp"
#include "pseudo_operators.hpp"

namespace metal {

PipelineBuilder::PipelineBuilder(
    std::shared_ptr<metal::OperatorFactory> registry,
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
  options.add_option("", "", "input", "Input",
                     cxxopts::value<std::string>()->default_value(""), "");

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

  options.parse_positional({"input"});

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
    if (DatagenOperator::isDatagenAgent(*agent) ||
        MetalCatOperator::isMetalCatAgent(*agent)) {
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
    for (auto &[op, agent] : orderedOperatorSpecsAndAgents) {
      if (op != nullptr) {
        operatorContexts.emplace_back(instantiateOperator(op, agent));
      }
    }

    result.pipeline = std::make_shared<Pipeline>(std::move(operatorContexts));
  }

  // Establish pipeline data source and sink
  result.dataSourceAgent = orderedOperatorSpecsAndAgents.front().second;
  result.dataSinkAgent = orderedOperatorSpecsAndAgents.back().second;

  // Validate datagen and metal_cat (if necessary)
  if (DatagenOperator::isDatagenAgent(*result.dataSourceAgent)) {
    DatagenOperator::validate(*result.dataSourceAgent);
    DatagenOperator::setInputFile(*result.dataSourceAgent);
  } else if (MetalCatOperator::isMetalCatAgent(*result.dataSourceAgent)) {
    MetalCatOperator::validate(
        *result.dataSourceAgent);  // TODO: validate is only called if metal_cat
                                   // is first agent
    MetalCatOperator::setInputFile(*result.dataSourceAgent);
  }

  // Create memory-mapped buffers if necessary, or establish file system context
  if (result.dataSourceAgent->internalInputFile().second == nullptr) {
    if (!result.dataSourceAgent->internalInputFilename().empty()) {
      result.dataSourceAgent->setInternalInputFile(
          result.dataSourceAgent->internalInputFilename());
    } else if (!DatagenOperator::isDatagenAgent(*result.dataSourceAgent)) {
      result.dataSourceAgent->createInputBuffer();
    }
  }
  if (!result.dataSinkAgent->internalOutputFilename().empty() 
       && !DevNullFile::isNullOutput(*result.dataSinkAgent)) {
    result.dataSinkAgent->setInternalOutputFile(
        result.dataSinkAgent->internalOutputFilename());
  } else {
    result.dataSinkAgent->createOutputBuffer();
  }

  // Tell agents that they're accepted
  for (auto &[op, agent] : orderedOperatorSpecsAndAgents) {
    (void)op;

    RegistrationResponse response;

    response.set_valid(true);

    if (agent->inputBuffer() != std::nullopt)
      response.set_input_buffer_filename(agent->inputBuffer()->filename());
    if (agent->outputBuffer() != std::nullopt)
      response.set_output_buffer_filename(agent->outputBuffer()->filename());

    if (agent->agentLoadFile().size())
      response.set_agent_read_filename(agent->agentLoadFile());

    agent->sendRegistrationResponse(response);
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

  auto input = parseResult["input"].as<std::string>();
  if (input.size()) {
    agent->setInputFile(input);
  }

  OperatorContext runtimeContext(std::move(userOperator));
  runtimeContext.setProfilingEnabled(parseResult["profile"].as<bool>());

  return runtimeContext;
}

}  // namespace metal
