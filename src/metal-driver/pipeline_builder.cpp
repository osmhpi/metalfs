#include <utility>

#include "pipeline_builder.hpp"

#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>

#include <metal-pipeline/pipeline_definition.hpp>
#include <metal-driver-messages/messages.hpp>
#include <metal-driver-messages/message_header.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-filesystem-pipeline/file_data_source.hpp>
#include <metal-filesystem-pipeline/file_data_sink.hpp>
#include <sys/stat.h>
#include <cxxopts.hpp>

namespace metal {

PipelineBuilder::PipelineBuilder(std::shared_ptr<metal::OperatorRegistry> registry, std::vector<std::shared_ptr<RegisteredAgent>> pipeline_agents)
        :  _registry(std::move(registry))
        , _pipeline_agents(std::move(pipeline_agents))
{
  for (const auto &op : _registry->operators()) {
    // TODO: Don't do this every time a pipeline starts
    _operatorOptions.insert(std::make_pair(op.first, buildOperatorOptions(op.second)));
  }
}


cxxopts::Options PipelineBuilder::buildOperatorOptions(const std::shared_ptr<AbstractOperator>& op) {

  auto options = cxxopts::Options(op->id(), op->description());

  options.add_option("", "h", "help", "Print help", cxxopts::value<bool>()->default_value("false"), "");
  options.add_option("", "p", "profile", "Enable profiling", cxxopts::value<bool>()->default_value("false"), "");

  for (const auto &keyOptionPair : op->optionDefinitions()) {
    const auto &option = keyOptionPair.second;
    switch (option.type()) {
      case OptionType::Bool: {
        options.add_option("", option.shrt(), option.key(), option.description(), cxxopts::value<bool>()->default_value("false"), "");
        break;
      }
      case OptionType::Uint: {
        options.add_option("", option.shrt(), option.key(), option.description(), cxxopts::value<uint32_t>(), "");
        break;
      }
      case OptionType::Buffer: {
        options.add_option("", option.shrt(), option.key(), option.description(), cxxopts::value<std::string>(), "");
        break;
      }
    }
  }

  return options;
}


std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> PipelineBuilder::resolve_operators() {

  // Check if each requested operator is used only once and look up the
  // operator specification

  std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> result;

  std::unordered_set<std::shared_ptr<AbstractOperator>> operators_with_agents;

  for (const auto &agent : _pipeline_agents) {
    auto op = _registry->operators().find(agent->operator_type);
    if (op == _registry->operators().end()) {
      throw std::runtime_error("Unknown operator was requested");
    }

    if (!operators_with_agents.emplace(op->second).second)
      throw std::runtime_error("Operator was used more than once");

    result.emplace_back(std::make_pair(op->second, agent));
  }

  return result;
}

std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> PipelineBuilder::configure() {

  auto operators = resolve_operators();

  for (auto &operatorAgentPair : operators) {
    set_operator_options_from_agent_request(operatorAgentPair.first, operatorAgentPair.second);
  }

  // Establish pipeline data source and sink
  std::deque<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> pipeline_operators;

  for (const auto &operatorAgentPair : operators)
    pipeline_operators.emplace_back(operatorAgentPair);

  auto dataSource = std::dynamic_pointer_cast<DataSource>(pipeline_operators.front().first);
  if (dataSource == nullptr) {
    auto dataSourceAgent = operators.front().second;
    if (!dataSourceAgent->internal_input_file.empty()) {
      dataSource = std::make_shared<FileDataSource>(dataSourceAgent->internal_input_file, 0, 0);
    } else {
      dataSourceAgent->input_buffer = Buffer::create_temp_file_for_shared_buffer(false);
      dataSource = std::make_shared<HostMemoryDataSource>(nullptr, 0); // TODO: Address and size are set later
    }
    pipeline_operators.emplace_front(std::make_pair(dataSource, nullptr));
  }

  auto dataSink = std::dynamic_pointer_cast<DataSink>(pipeline_operators.back().first);
  if (dataSink == nullptr) {  // To my knowledge, there are no user-accessible data sinks, so always true
    auto dataSinkAgent = operators.back().second;
    if (!dataSinkAgent->internal_output_file.empty()) {
      if (dataSinkAgent->internal_output_file == "$NULL") {
        dataSink = std::make_shared<NullDataSink>(0);
      } else {
        dataSink = std::make_shared<FileDataSink>(dataSinkAgent->internal_output_file, 0, 0);
      }
    } else {
      dataSinkAgent->output_buffer = Buffer::create_temp_file_for_shared_buffer(true);
      dataSink = std::make_shared<HostMemoryDataSink>(nullptr, 0);  // TODO: Address and size are set later
    }
    pipeline_operators.emplace_back(std::make_pair(dataSink, nullptr));
  }

  // Tell agents that they're accepted
  for (auto &operatorAgentPair : operators) {
    ServerAcceptAgent response;

    response.set_valid(true);

    if (operatorAgentPair.second->input_buffer != std::nullopt)
      response.set_input_buffer_filename(operatorAgentPair.second->input_buffer.value().filename());
    if (operatorAgentPair.second->output_buffer != std::nullopt)
      response.set_output_buffer_filename(operatorAgentPair.second->output_buffer.value().filename());

    operatorAgentPair.second->socket.send_message<message_type::ServerAcceptAgent>(response);
  }

  std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> result;
  result.reserve(pipeline_operators.size());

  for (const auto &op : pipeline_operators)
    result.emplace_back(op);

  return result;
}

void PipelineBuilder::set_operator_options_from_agent_request(
        std::shared_ptr<AbstractOperator> op,
        std::shared_ptr<RegisteredAgent> agent) {

  auto &options = _operatorOptions.at(op->id());

  // Contain some C++ / C interop ugliness inside here...

  std::vector<char*> argsRaw;
  for (const auto &arg : agent->args)
    argsRaw.emplace_back(const_cast<char*>(arg.c_str()));

  int argc = (int)agent->args.size();
  char **argv = argsRaw.data();
  auto parseResult = options.parse(argc, argv);

  if (parseResult["help"].as<bool>()) {
      // Not really an exception, but the fastest way how we can exit the processing flow
      throw ClientError(agent, options.help());
  }

  op->set_profiling_enabled(parseResult["profile"].as<bool>());

  for (const auto &optionType : op->optionDefinitions()) {
    auto result = parseResult[optionType.first];

    switch (optionType.second.type()) {
      case OptionType::Uint: {
        op->setOption(optionType.first, result.as<uint32_t>());
        break;
      }
      case OptionType::Bool: {
        op->setOption(optionType.first, result.as<bool>());
        break;
      }
      case OptionType::Buffer: {
        if (!optionType.second.bufferSize().has_value())
          throw std::runtime_error("File option metadata not found");

        auto filePath = resolvePath(result.as<std::string>(), agent->cwd);

        // TODO: Detect if path points to FPGA file

        auto buffer = std::make_unique<std::vector<char>>(optionType.second.bufferSize().value());

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

        op->setOption(optionType.first, std::move(buffer));
      }
    }
  }
}

std::string PipelineBuilder::resolvePath(std::string relative_or_absolute_path, std::string working_dir) {
  if (relative_or_absolute_path.size() && relative_or_absolute_path[0] == '/') {
    // is absolute
    return relative_or_absolute_path;
  }

  return working_dir + "/" + relative_or_absolute_path;
}


} // namespace metal
