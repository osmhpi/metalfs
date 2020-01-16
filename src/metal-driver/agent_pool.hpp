#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "registered_agent.hpp"

namespace metal {

class RegistrationRequest;

class AgentPool {
 public:
  void registerAgent(RegistrationRequest &hello, Socket socket);
  bool containsValidPipeline();
  void releaseUnusedAgents();
  void reset();
  std::vector<std::shared_ptr<RegisteredAgent>> cachedPipeline() {
    return _pipeline_agents;
  }

 protected:
  void sendAllAgentsInvalid(
      std::unordered_set<std::shared_ptr<RegisteredAgent>> &agents);
  void sendRegistrationInvalid(RegisteredAgent &agent);

  std::unordered_set<std::shared_ptr<RegisteredAgent>> _registered_agents;
  std::vector<std::shared_ptr<RegisteredAgent>> _pipeline_agents;
  bool _contains_valid_pipeline_cached;
};

}  // namespace metal
