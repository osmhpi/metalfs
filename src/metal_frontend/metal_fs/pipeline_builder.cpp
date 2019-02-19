#include "pipeline_builder.hpp"

#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>

#include <metal_pipeline/pipeline_definition.hpp>
#include <Messages.pb.h>
#include <metal_frontend/messages/message_header.hpp>
#include <metal_pipeline/data_source.hpp>
#include <metal_pipeline/data_sink.hpp>
#include <metal_filesystem_pipeline/file_data_source.hpp>
#include <metal_filesystem_pipeline/file_data_sink.hpp>
#include "../../../third_party/cxxopts/include/cxxopts.hpp"

namespace metal {

PipelineBuilder::PipelineBuilder(std::unordered_set<std::shared_ptr<RegisteredAgent>> pipeline_agents)
        : _pipeline_agents(std::move(pipeline_agents))
        , _registry(OperatorRegistry("./operators"))
{
  for (const auto &op : _registry.operators()) {
    _operatorOptions[op.first] = buildOperatorOptions(op.second);
  }
}


cxxopts::Options PipelineBuilder::buildOperatorOptions(std::shared_ptr<UserOperator> op) {

  auto options = cxxopts::Options(op->id(), op->description());

  for (const auto &keyOptionPair : op->optionDefinitions()) {
    const auto &option = keyOptionPair.second;
    switch (option.type()) {
      case OptionType::BOOL: {
        options.add_option("", option.shrt(), option.key(), option.description(), cxxopts::value<bool>(), "");
        break;
      }
      case OptionType::INT: {
        options.add_option("", option.shrt(), option.key(), option.description(), cxxopts::value<int>(), "");
        break;
      }
      case OptionType::BUFFER: {
        options.add_option("", option.shrt(), option.key(), option.description(), cxxopts::value<std::string>(), "");
        break;
      }
    }
  }

  return options;
}


bool PipelineBuilder::validate() {
  // Check if each requested operator is used only once and look up the
  // operator specification
  bool valid = true;
  uint64_t pipeline_length = 0;
  for (const auto &agent : _pipeline_agents) {
    auto agents_with_same_operator = std::count(_pipeline_agents.begin(), _pipeline_agents.end(), [&] (const std::shared_ptr<RegisteredAgent> & a) {
      return a->operator_type == agent->operator_type;
    });

    if (agents_with_same_operator > 1) {
      valid = false;
      break;
    }

    auto op = _registry.operators().find(agent->operator_type);
    if (op == _registry.operators().end()) {
      valid = false;
      break;
    }

    ++pipeline_length;
  }

  return valid;
}

std::shared_ptr<PipelineDefinition> PipelineBuilder::configure() {

  std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> operators;

  for (auto &operatorAgentPair : operators) {
    set_operator_options_from_agent_request(operatorAgentPair.first, operatorAgentPair.second);
  }

  // TODO: catch...


  for (auto &operatorAgentPair : operators) {
    ServerAcceptAgent response;
    response.set_valid(valid);
//            response.set_error_msg(responses[])

    char buf[response.ByteSize()];
    response.SerializeToArray(buf, response.ByteSize());

    MessageHeader header(message_type::SERVER_ACCEPT_AGENT, response.ByteSize());
    header.sendHeader(operatorAgentPair.second->socket);
    send(operatorAgentPair.second->socket, buf, response.ByteSize(), 0);
  }

//        catch: if (!valid) {
//             while (!IsListEmpty(&pipeline_agents)) {
//                registered_agent_t *free_agent = CONTAINING_LIST_RECORD(pipeline_agents.Flink, registered_agent_t);
//                RemoveEntryList(&free_agent->Link);
//                free_registered_agent(free_agent);
//            }
//            continue;
//        }


  std::vector<std::shared_ptr<AbstractOperator>> operatorsOnly;

  for (const auto &operatorAgentPair : operators)
    operatorsOnly.emplace_back(operatorAgentPair.first);

  auto dataSource = std::dynamic_pointer_cast<DataSource>(operators.front().first);
  if (dataSource == nullptr) {
    if (!operators.front().second->internal_input_file.empty()) {
      dataSource = std::make_shared<FileDataSource>(operators.front().second->internal_input_file);
    } else {
      dataSource = std::make_shared<HostMemoryDataSource>(0, 0);
    }
    // TODO: Add to vector
  }

  auto dataSink = std::dynamic_pointer_cast<DataSink>(operators.back().first);
  if (dataSink == nullptr) {  // To my knowledge, there are no user-accessible data sinks, so always true
    if (!operators.back().second->internal_output_file.empty()) {
      if (operators.back().second->internal_output_file == "$NULL")
        dataSink = std::make_shared<NullDataSink>(0 /* TODO */);
      else
        dataSink = std::make_shared<FileDataSink>(operators.back().second->internal_output_file);
    } else {
      dataSink = std::make_shared<HostMemoryDataSink>(0, 0);
    }
    operators.emplace_back(dataSink);
  }

  auto pipeline = std::make_shared<PipelineDefinition>(operatorsOnly);
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

  for (const auto &argument : parseResult.arguments()) {
    auto optionType = op->optionDefinitions().find(argument.key())
    if (optionType == op->optionDefinitions().end())
      throw std::runtime_error("Option was not found");

    switch (optionType->type()) {
      case OptionType::INT: {
        op->setOption(argument.key(), argument.as<int>());
        break;
      }
      case OptionType::BOOL: {
        op->setOption(argument.key(), argument.as<bool>());
        break;
      }
      case OptionType::BUFFER: {
        if (!optionType->bufferSize().hasValue())
          throw std::runtime_error("File option metadata not found");

        auto filePath = resolvePath(argument.value(), agent->cwd);

        // TODO: Detect if path points to FPGA file

        auto buffer = std::make_unique<std::vector<char>>(optionType->bufferSize().value());

        auto fp = fopen(filePath, "r");
        if (fp == nullptr) {
          throw std::runtime_error("Could not open file");
        }

        auto nread = fread(buffer.get(), 1, buffer->size(), fp);
        if (nread <= 0) {
          fclose(fp);
          throw std::runtime_error("Could not read from file");
        }

        fclose(fp);

        setOption(argument.key(), std::move(buffer));
      }
    }
  }
}

std::string PipelineBuilder::resolvePath(std::string relative_or_absolue_path, std::string working_dir) {
  auto dir = opendir(working_dir.c_str());
  if (dir == nullptr) {
    throw std::runtime_error("Could not open working directory of symbolic executable process");
  }

  auto dirf = dirfd(dir);
  if (dirf == -1) {
    closedir(dir);
    throw std::runtime_error("Could not obtain dirfd");
  }

  // Determine the size of the buffer (inspired from man readlink(2) example)
  struct stat sb{};
  char *buf;
  ssize_t nbytes, bufsiz;

  if (lstat(relative_or_absolue_path.c_str(), &sb) == -1) {
    closedir(dir);
    throw std::runtime_error("Could not obtain file status");
  }

  bufsiz = sb.st_size + 1;
  if (sb.st_size == 0)
    bufsiz = PATH_MAX;

  buf = static_cast<char *>(malloc(static_cast<size_t>(bufsiz)));
  if (buf == nullptr) {
    closedir(dir);
    throw std::runtime_error("Allocation failure");
  }

  nbytes = readlinkat(dirf, relative_or_absolue_path.c_str(), buf, static_cast<size_t>(bufsiz));
  if (nbytes == -1) {
    closedir(dir);
    free(buf);
    throw std::runtime_error("Could not resolve path");
  }

  closedir(dir);
  std::string result = buf;
  free(buf);
  return result;
}


} // namespace metal
