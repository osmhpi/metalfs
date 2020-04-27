#pragma once

#include <string>
#include <unordered_set>
#include <utility>

#include <metal-driver-messages/messages.hpp>
#include <metal-pipeline/card.hpp>
#include <metal-pipeline/operator_factory.hpp>

#include "agent_pool.hpp"

void* start_socket(void* args);

namespace metal {

class MessageHeader;

class Server {
 public:
  explicit Server(std::shared_ptr<OperatorFactory> registry);
  virtual ~Server();

  void start(Card card);

  const std::string& socketFilename() { return _socketFileName; }

 protected:
  void processRequest(Socket socket, Card card);

  AgentPool _agents;
  std::string _socketFileName;
  std::shared_ptr<metal::OperatorFactory> _registry;
  int _listenfd;
};
}  // namespace metal
