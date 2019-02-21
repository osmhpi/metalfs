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
#include "registered_agent.hpp"
#include "server.hpp"
#include "Messages.pb.h"
#include <metal_frontend/messages/message_header.hpp>
#include "agent_pool.hpp"
#include "pipeline_builder.hpp"
#include "pipeline_loop.hpp"
#include "../common/socket.hpp"

namespace metal {


Server::Server(std::string socketFileName) : _socketFileName(std::move(socketFileName)), _listenfd(0) {}


Server::~Server() {
    close(_listenfd);
}

void metal::Server::start(void *args) {
    std::string socket_file_name = (char*)args;

    Server server(socket_file_name);
    server.startInternal();
}

void Server::startInternal() {

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

        process_request(connfd);
    }
}

void Server::process_request(int connfd) {
    try {
        Socket socket(connfd);
        auto request = socket.receive_message<message_type::AGENT_HELLO, ClientHello>();
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
        PipelineBuilder builder(pipeline);

        auto operators_agents = builder.configure();

        PipelineLoop loop(std::move(operators_agents));
        loop.run();
    } catch (std::exception &ex) {
        // Something went wrong.
        // Maybe print the error message...
    }

    _agents.reset();
}

} // namespace metal
