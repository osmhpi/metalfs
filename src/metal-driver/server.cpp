#include "server.hpp"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <algorithm>
#include <utility>

#include <google/protobuf/io/zero_copy_stream.h>
#include <spdlog/spdlog.h>

#include <metal-driver-messages/buffer.hpp>
#include <metal-driver-messages/message_header.hpp>
#include <metal-driver-messages/messages.hpp>
#include <metal-driver-messages/socket.hpp>
#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator_factory.hpp>

#include "agent_pool.hpp"
#include "operator_agent.hpp"
#include "pipeline_builder.hpp"
#include "pipeline_loop.hpp"

namespace metal {

Server::Server(std::shared_ptr<OperatorFactory> registry)
    : _socketFileName(), _registry(std::move(registry)), _listenfd(0) {
  char socket_dir[] = "/tmp/metal-socket-XXXXXX";
  if (mkdtemp(socket_dir) == nullptr) {
    throw std::runtime_error("Could not create temporary directory.");
  }
  _socketFileName = std::string(socket_dir) + "/metal.sock";
}

Server::~Server() { close(_listenfd); }

void Server::start(int card) {
  _listenfd = 0;
  int connfd = 0;
  struct sockaddr_un serv_addr {};

  _listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
  serv_addr.sun_family = AF_UNIX;
  strcpy(serv_addr.sun_path, _socketFileName.c_str());

  bind(_listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

  listen(_listenfd, 10);  // 10 = Queue len

  for (;;) {
    connfd = accept(_listenfd, NULL, NULL);

    processRequest(Socket(connfd), card);
  }
}

void Server::processRequest(Socket socket, int card) {
  try {
    _agents.registerAgent(std::move(socket));
  } catch (std::exception &ex) {
    // Don't know this guy -- doesn't even say hello
    spdlog::warn(ex.what());
    return;
  }

  if (!_agents.containsValidPipeline()) {
    return;
  }

  auto pipeline = _agents.cachedPipeline();
  _agents.releaseUnusedAgents();

  try {
    PipelineBuilder builder(_registry, pipeline);

    auto configuredPipeline = builder.configure();

    PipelineLoop loop(std::move(configuredPipeline), card);
    loop.run();
  } catch (ClientError &error) {
    error.agent()->setError(error.what());
  } catch (std::exception &ex) {
    // Something went wrong.
    spdlog::error("An error occurred during pipeline execution: {}", ex.what());
  }

  _agents.reset();
}

}  // namespace metal
