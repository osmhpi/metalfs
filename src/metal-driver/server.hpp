#include <utility>

#pragma once

#include <metal-driver-messages/messages.hpp>
#include <metal-pipeline/operator_registry.hpp>
#include <string>
#include <unordered_set>
#include "agent_pool.hpp"

void* start_socket(void* args);

namespace metal {

class MessageHeader;

class Server {
 public:
  explicit Server(std::string socketFileName,
                  std::shared_ptr<OperatorRegistry> registry);
  virtual ~Server();

  static void start(const std::string& socket_file_name,
                    std::shared_ptr<OperatorRegistry> registry, int card);

 protected:
  void startInternal(int card);
  void processRequest(Socket socket, int card);

  AgentPool _agents;
  std::string _socketFileName;
  std::shared_ptr<metal::OperatorRegistry> _registry;
  int _listenfd;
};
}  // namespace metal
