#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pthread.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <algorithm>
#include <metal_pipeline/abstract_operator.hpp>
#include <metal_pipeline/operator_registry.hpp>
#include <metal_pipeline/pipeline_runner.hpp>
#include <dirent.h>
#include <sys/stat.h>
#include <metal_pipeline/data_source.hpp>
#include <metal_pipeline/data_sink.hpp>
#include <metal_filesystem_pipeline/file_data_source.hpp>
#include <metal_filesystem_pipeline/file_data_sink.hpp>
#include <metal_filesystem_pipeline/filesystem_pipeline_runner.hpp>

#include "../../metal/metal.h"

#include "../../metal_operators/op_read_file/operator.h"
#include "../../metal_operators/op_read_mem/operator.h"
#include "../../metal_operators/op_write_file/operator.h"
#include "../../metal_operators/op_write_mem/operator.h"
#include "../../metal_operators/op_change_case/operator.h"

#include "metal_frontend/common/buffer.hpp"
#include "../common/message.h"
#include "../common/known_operators.h"
#include "list/list.h"
#include "registered_agent.h"
#include "server.hpp"
#include "Messages.pb.h"

//void update_agent_wiring() {
//    if (IsListEmpty(&registered_agents))
//        return;
//
//    PLIST_ENTRY current_link = registered_agents.Flink;
//
//    do {
//        registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);
//
//        if (current_agent->input_agent_pid && current_agent->input_agent == NULL) {
//            current_agent->input_agent = find_agent_with_pid(current_agent->input_agent_pid);
//            if (current_agent->input_agent != NULL)
//                current_agent->input_agent->output_agent = current_agent;
//        }
//
//        current_link = current_link->Flink;
//    } while (current_link != &registered_agents);
//}



namespace metal {


class MessageHeader {
public:
    MessageHeader() : MessageHeader(static_cast<message_type>(0), 0) {}
    MessageHeader(message_type type, int32_t length) : _type(static_cast<int>(type)), _length(htonl(length)) {}

    message_type type() { return static_cast<message_type>(_type); }
    size_t length() { return static_cast<size_t>(_length); }

private:
    int _type;
    int32_t _length;
};


void metal::Server::start(void *args) {
    std::string socket_file_name = (char*)args;

    Server server(socket_file_name);
    server.startInternal();
}

void Server::startInternal() {

    int listenfd = 0, connfd = 0;
    struct sockaddr_un serv_addr{};

    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, _socketFileName.c_str());

    bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10); // 10 = Queue len

    for (;;) {
        connfd = accept(listenfd, NULL, NULL);

        process_request(connfd);
    }

    // TODO: Move to destructor
    close(listenfd);
}

void Server::process_request(int connfd) {
    MessageHeader incoming_message;
    recv(connfd, &incoming_message, sizeof(incoming_message), 0);

    if (incoming_message.type() == message_type::AGENT_HELLO) {
//        char buffer [incoming_message.length()];
//        recv(connfd, buffer, incoming_message.length(), 0);
//
//        ClientHello request;
//        request.ParseFromArray(buffer, incoming_message.length());

        auto request = receive_message<ClientHello>(connfd, incoming_message);

        // Add the agent to the list of registered agents
        register_agent(request, connfd);

        // Try to derive a full pipeline
        derive_execution_plan();
        if (_pipeline_agents.empty()) {
            return;
        }

        // Tell all remaining agents to go home
        send_all_agents_invalid(_registered_agents);

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

        if (!valid) {
            send_all_agents_invalid(_pipeline_agents);
            return;
        }

        // TODO
        std::vector<std::pair<std::shared_ptr<AbstractOperator>, std::shared_ptr<RegisteredAgent>>> operators;

        for (auto &operatorAgentPair : operators) {
            setOperatorOptionsFromInvocationData(operatorAgentPair.first, operatorAgentPair.second);
        }

        // TODO: catch...


        for (auto &operatorAgentPair : operators) {
            ServerAcceptAgent response;
            response.set_valid(valid);
//            response.set_error_msg(responses[])

            char buf[response.ByteSize()];
            response.SerializeToArray(buf, response.ByteSize());

            MessageHeader header(message_type::SERVER_ACCEPT_AGENT, response.ByteSize());
            send(operatorAgentPair.second->socket, &header, sizeof(header), 0);
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

        FilesystemPipelineRunner runner(pipeline);

        // Wait until all clients signal ready
        ClientPushBuffer inputBufferMessage;
        for (auto &operatorAgentPair : operators) {
            MessageHeader req;
            recv(operatorAgentPair.second->socket, &req, sizeof(req), 0);
            if (req.type() != message_type::AGENT_PUSH_BUFFER)
                throw std::runtime_error("Unexpected message");

            auto msg = receive_message<ClientPushBuffer>(operatorAgentPair.second->socket, req);
        }

        for (;;) {
            ServerProcessedBuffer processing_response;

            size_t size = 0;
            bool eof = false;

            size_t output_size = 0;

            // If the client provides us with input data, forward it to the pipeline
            if (operators.front().second->input_buffer) {
                size = inputBufferMessage.size();
                eof = inputBufferMessage.eof();
            }

            if (size) {
                runner.run(eof);
            }

            // Send a processing response for the input agent (eof, perfmon)

            // Send a processing response for the output agent (eof, perfmon, size)


            if (eof) {
                // Send a processing response to all other agents (eof, perfmon)
            }

            if (operators.back().second->output_buffer) {
                // Wait for output client push buffer message (i.e. 'ready')
            }

            if (operators.front().second->input_buffer) {
                // Wait for output client push buffer message (i.e. the next inputBufferMessage)
            }
        }

        for (const auto &a : _pipeline_agents) {
            close(a->socket);
        }

        _pipeline_agents.clear();
    } else {
        // Don't know this guy -- doesn't even say hello
        close(connfd);
    }
}

void Server::register_agent(ClientHello &request, int connfd) {
    auto agent = std::make_shared<RegisteredAgent>();

    agent->socket = connfd;
    agent->pid = request.pid();
    agent->operator_type = request.operator_type();
    agent->input_agent_pid = request.input_pid();
    agent->output_agent_pid = request.output_pid();
    agent->args.reserve(request.args().size());
    for (const auto& arg : request.args())
        agent->args.emplace_back(arg);

    agent->cwd = request.cwd();
    agent->metal_mountpoint = request.metal_mountpoint();

    if (request.has_metal_input_filename())
        agent->internal_input_file = request.metal_input_filename();
    if (request.has_metal_output_filename())
        agent->internal_output_file = request.metal_output_filename();

    // If the agent's input is not connected to another agent, it should
    // have provided a file that will be used for memory-mapped data exchange
    // except when we're writing to an internal file
    if (agent->input_agent_pid == 0 && agent->internal_input_file.empty()) {
        if (request.input_buffer_filename().empty()) {
            // Should not happen
            close(connfd);
            return;
        }

        // TODO: Same procedure for output buffer
        map_shared_buffer_for_reading(
                request.input_buffer_filename(),
                &agent->input_file, &agent->input_buffer
        );
    }

    _registered_agents.emplace(agent);

    update_agent_wiring();
}

void Server::derive_execution_plan() {
    // Looks for agents at the beginning of a pipeline
    // that have not been signaled yet

    auto pipelineStart = *std::find_if(_registered_agents.begin(), _registered_agents.end(), [](const std::shared_ptr<RegisteredAgent> & a) {
        return a->input_agent_pid == 0;
    });

    // Walk the pipeline
    std::shared_ptr<RegisteredAgent> pipeline_agent = pipelineStart;
    uint64_t pipeline_length = 0;
    while (pipeline_agent && pipeline_agent->output_agent_pid != 0) {
        pipeline_agent = pipeline_agent->output_agent.lock();
        ++pipeline_length;
    }

    if (pipeline_agent) {
        // We have found an agent with output_agent_pid == 0, meaning it's an external file or pipe
        // So we have found a complete pipeline!
        // Now, remove all agents that are part of the pipeline from registered_agents and instead
        // insert them into pipeline_agents
        pipeline_agent = pipelineStart;
        while (pipeline_agent) {
            _registered_agents.erase(pipeline_agent);
            _pipeline_agents.insert(pipeline_agent);

            if (pipeline_agent->output_agent_pid == 0)
                break;

            pipeline_agent = pipeline_agent->output_agent.lock();
        }
    }
}

void Server::send_all_agents_invalid(std::unordered_set<std::shared_ptr<RegisteredAgent>> &agents) {
    for (const auto &agent : agents) {
        send_agent_invalid(*agent);
    }

    agents.clear();
}

void Server::send_agent_invalid(RegisteredAgent &agent) {
    metal::ServerAcceptAgent accept_data;
    accept_data.set_error_msg("An invalid operator chain was specified.\n");
    accept_data.set_valid(false);

    MessageHeader header {message_type::SERVER_ACCEPT_AGENT, accept_data.ByteSize() };
    send(agent.socket, &header, sizeof(header), 0);
    send(agent.socket, &accept_data, header.length(), 0);
}

Server::Server(std::string socketFileName) : _socketFileName(std::move(socketFileName)), _registry(OperatorRegistry("./operators")) {
    for (const auto &op : _registry.operators()) {
        _operatorOptions[op.first] = buildOperatorOptions(op.second);
    }
}

void Server::setOperatorOptionsFromInvocationData(
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

std::string Server::resolvePath(std::string relative_or_absolue_path, std::string working_dir) {
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

cxxopts::Options Server::buildOperatorOptions(std::shared_ptr<UserOperator> op) {

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

template<typename T>
T Server::receive_message(int connfd, MessageHeader &header) {
    char buffer [header.length()];
    recv(connfd, buffer, header.length(), 0);

    T message;
    message.ParseFromArray(buffer, header.length());
    return message;
}

} // namespace metal