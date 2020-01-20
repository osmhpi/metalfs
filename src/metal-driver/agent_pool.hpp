#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "operator_agent.hpp"

namespace metal {

class RegistrationRequest;

class AgentPool {
 public:
  void registerAgent(Socket socket);
  bool containsValidPipeline();
  void releaseUnusedAgents();
  void reset();
  std::vector<std::shared_ptr<OperatorAgent>> cachedPipeline() {
    return _pipeline_agents;
  }

 protected:
  void sendAllAgentsInvalid(
      std::unordered_set<std::shared_ptr<OperatorAgent>> &agents);
  void sendRegistrationInvalid(OperatorAgent &agent);

  std::unordered_set<std::shared_ptr<OperatorAgent>> _registered_agents;
  std::vector<std::shared_ptr<OperatorAgent>> _pipeline_agents;
  bool _contains_valid_pipeline_cached;
};

}  // namespace metal
