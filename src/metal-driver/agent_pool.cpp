#include <unistd.h>

#include <algorithm>

#include <spdlog/spdlog.h>

#include <metal-driver-messages/buffer.hpp>
#include <metal-driver-messages/message_header.hpp>

#include "agent_pool.hpp"
#include "registered_agent.hpp"
#include "server.hpp"

namespace metal {

void AgentPool::registerAgent(Socket socket) {
  auto agent = std::make_shared<RegisteredAgent>(std::move(socket));

  for (auto &other_agent : _registered_agents) {
    if (other_agent->isOutputConnectedTo(*agent)) {
      other_agent->setOutputAgent(agent);
    } else if (agent->isOutputConnectedTo(*other_agent)) {
      agent->setOutputAgent(other_agent);
    }
  }
  _registered_agents.emplace(agent);

  _contains_valid_pipeline_cached = false;
}

bool AgentPool::containsValidPipeline() {
  if (_contains_valid_pipeline_cached) return true;

  _contains_valid_pipeline_cached = false;

  auto pipelineStart =
      std::find_if(_registered_agents.begin(), _registered_agents.end(),
                   [](const std::shared_ptr<RegisteredAgent> &a) {
                     return a->isInputAgent();
                   });

  if (pipelineStart == _registered_agents.end()) {
    return false;
  }

  // Walk the pipeline until we find an agent with output_pid == 0
  std::vector<std::shared_ptr<RegisteredAgent>> pipeline_agents;

  auto pipeline_agent = *pipelineStart;
  while (pipeline_agent) {
    pipeline_agents.emplace_back(pipeline_agent);

    if (pipeline_agent->isOutputAgent()) break;

    pipeline_agent = pipeline_agent->outputAgent();
  }

  if (pipeline_agents.back()->isOutputAgent()) {
    _contains_valid_pipeline_cached = true;
    _pipeline_agents = std::move(pipeline_agents);
    for (const auto &agent : _pipeline_agents) {
      _registered_agents.erase(agent);
    }
  }

  return _contains_valid_pipeline_cached;
}

void AgentPool::releaseUnusedAgents() {
  sendAllAgentsInvalid(_registered_agents);
}

void AgentPool::sendAllAgentsInvalid(
    std::unordered_set<std::shared_ptr<RegisteredAgent>> &agents) {
  for (const auto &agent : agents) {
    sendRegistrationInvalid(*agent);
  }

  agents.clear();
}

void AgentPool::sendRegistrationInvalid(RegisteredAgent &agent) {
  metal::RegistrationResponse accept_data;

  if (!agent.error().empty()) {
    accept_data.set_error_msg(agent.error());
  } else {
    accept_data.set_error_msg("An invalid operator chain was specified.\n");
  }

  accept_data.set_valid(false);

  agent.sendRegistrationResponse(accept_data);
  agent.setTerminated();
}

void AgentPool::reset() {
  for (const auto &agent : _pipeline_agents) {
    if (!agent->terminated()) {
      sendRegistrationInvalid(*agent);
    }
  }

  _pipeline_agents.clear();

  sendAllAgentsInvalid(_registered_agents);
}

}  // namespace metal
