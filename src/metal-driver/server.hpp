#pragma once

#include <string>
#include <unordered_set>
#include <utility>

#include <metal-driver-messages/messages.hpp>
#include <metal-pipeline/fpga_action_factory.hpp>
#include <metal-pipeline/operator_factory.hpp>

#include "agent_pool.hpp"

void* start_socket(void* args);

namespace metal {

class MessageHeader;

class Server {
 public:
  explicit Server(std::shared_ptr<OperatorFactory> registry);
  virtual ~Server();

  void start(std::shared_ptr<FpgaActionFactory> actionFactory);

  const std::string& socketFilename() { return _socketFileName; }

 protected:
  void processRequest(Socket socket, std::shared_ptr<FpgaActionFactory> actionFactory);

  AgentPool _agents;
  std::string _socketFileName;
  std::shared_ptr<metal::OperatorFactory> _registry;
  int _listenfd;
};
}  // namespace metal
