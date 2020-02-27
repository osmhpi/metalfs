#include <utility>

#pragma once

#include <metal-driver-messages/messages.hpp>
#include <metal-pipeline/operator_factory.hpp>
#include <string>
#include <unordered_set>
#include "agent_pool.hpp"

void* start_socket(void* args);

namespace metal {

class MessageHeader;

class Server {
 public:
  explicit Server(std::shared_ptr<OperatorFactory> registry);
  virtual ~Server();

  void start(int card);

  const std::string &socketFilename() { return _socketFileName; }

 protected:
  void startInternal(int card);
  void processRequest(Socket socket, int card);

  AgentPool _agents;
  std::string _socketFileName;
  std::shared_ptr<metal::OperatorFactory> _registry;
  int _listenfd;
};
}  // namespace metal
