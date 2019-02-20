#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "registered_agent.hpp"

namespace metal {

class ClientHello;

class AgentPool {
 public:

  void register_agent(ClientHello &hello, int socket);
  bool contains_valid_pipeline();
  void release_unused_agents();
  void reset();
  std::vector<std::shared_ptr<RegisteredAgent>> cached_pipeline_agents() { return _pipeline_agents; }

 protected:
  void send_all_agents_invalid(std::unordered_set<std::shared_ptr<RegisteredAgent>> &agents);
  void send_agent_invalid(RegisteredAgent &agent);

  std::unordered_set<std::shared_ptr<RegisteredAgent>> _registered_agents;
  std::vector<std::shared_ptr<RegisteredAgent>> _pipeline_agents;
  bool _contains_valid_pipeline_cached;
};

} // namespace metal
