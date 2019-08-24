#include <utility>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pthread.h>

#include <google/protobuf/io/zero_copy_stream.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <metal_pipeline/abstract_operator.hpp>
#include <metal_pipeline/operator_registry.hpp>
#include <metal_pipeline/pipeline_runner.hpp>
#include <metal_pipeline/data_source.hpp>
#include <metal_pipeline/data_sink.hpp>
#include <metal_filesystem_pipeline/file_data_source.hpp>
#include <metal_filesystem_pipeline/file_data_sink.hpp>

#include <metal_frontend/messages/buffer.hpp>
#include "registered_agent.hpp"
#include "server.hpp"
#include "Messages.pb.h"
#include <metal_frontend/messages/message_header.hpp>
#include "agent_pool.hpp"
#include "pipeline_builder.hpp"
#include "pipeline_loop.hpp"
#include "../messages/socket.hpp"

namespace metal {


Server::Server(std::string socketFileName, std::shared_ptr<OperatorRegistry> registry) : _socketFileName(std::move(socketFileName)), _registry(std::move(registry)), _listenfd(0) {}


Server::~Server() {
    close(_listenfd);
}

void Server::start(const std::string& socket_file_name, std::shared_ptr<OperatorRegistry> registry, int card) {
    Server server(socket_file_name, std::move(registry));
    server.startInternal(card);
}

void Server::startInternal(int card) {

    _listenfd = 0;
    int connfd = 0;
    struct sockaddr_un serv_addr{};

    _listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, _socketFileName.c_str());

    bind(_listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(_listenfd, 10); // 10 = Queue len

    for (;;) {
        connfd = accept(_listenfd, NULL, NULL);

        process_request(connfd, card);
    }
}

void Server::process_request(int connfd, int card) {
    try {
        Socket socket(connfd);
        auto request = socket.receive_message<message_type::AgentHello>();
        _agents.register_agent(request, connfd);
    } catch (std::exception& ex) {
        // Don't know this guy -- doesn't even say hello
        close(connfd);
        return;
    }

    if (!_agents.contains_valid_pipeline()) {
        return;
    }

    auto pipeline = _agents.cached_pipeline_agents();
    _agents.release_unused_agents();

    try {
        PipelineBuilder builder(_registry, pipeline);

        auto operators_agents = builder.configure();

        PipelineLoop loop(std::move(operators_agents), card);
        loop.run();
    } catch (ClientError &error) {
        error.agent()->error = error.what();
    } catch (std::exception &ex) {
        // Something went wrong.
        spdlog::error("An error occurred during pipeline execution: {}", ex.what());
    }

    _agents.reset();
}

} // namespace metal
