#include <unistd.h>

#include <algorithm>

#include <spdlog/spdlog.h>

#include <metal-driver-messages/buffer.hpp>
#include <metal-driver-messages/message_header.hpp>

#include "agent_pool.hpp"
#include "registered_agent.hpp"
#include "server.hpp"

namespace metal {

void AgentPool::registerAgent(RegistrationRequest &hello, Socket socket) {
  auto agent = std::make_shared<RegisteredAgent>(std::move(socket));

  agent->pid = hello.pid();
  agent->operator_type = hello.operator_type();
  agent->input_agent_pid = hello.input_pid();
  agent->output_agent_pid = hello.output_pid();
  agent->args.reserve(hello.args().size());
  for (const auto &arg : hello.args()) {
    agent->args.emplace_back(arg);
  }

  agent->cwd = hello.cwd();
  agent->metal_mountpoint = hello.metal_mountpoint();

  if (hello.has_metal_input_filename())
    agent->internal_input_file = hello.metal_input_filename();
  if (hello.has_metal_output_filename())
    agent->internal_output_file = hello.metal_output_filename();

  for (auto &other_agent : _registered_agents) {
    if (other_agent->output_agent_pid == agent->pid) {
      other_agent->output_agent = agent;
    } else if (other_agent->pid == agent->output_agent_pid) {
      agent->output_agent = other_agent;
    }
  }

  _registered_agents.emplace(agent);

  _contains_valid_pipeline_cached = false;
}

bool AgentPool::containsValidPipeline() {
  if (_contains_valid_pipeline_cached) return true;

  _contains_valid_pipeline_cached = false;

  auto pipelineStart =
      *std::find_if(_registered_agents.begin(), _registered_agents.end(),
                    [](const std::shared_ptr<RegisteredAgent> &a) {
                      return a->input_agent_pid == 0;
                    });

  if (!pipelineStart) return false;

  // Walk the pipeline until we find an agent with output_pid == 0
  std::vector<std::shared_ptr<RegisteredAgent>> pipeline_agents;

  auto pipeline_agent = pipelineStart;
  while (pipeline_agent) {
    pipeline_agents.emplace_back(pipeline_agent);

    if (pipeline_agent->output_agent_pid == 0) break;

    pipeline_agent = pipeline_agent->output_agent;
  }

  if (pipeline_agents.back()->output_agent_pid == 0) {
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

  if (!agent.error.empty()) {
    accept_data.set_error_msg(agent.error);
  } else {
    accept_data.set_error_msg("An invalid operator chain was specified.\n");
  }

  accept_data.set_valid(false);

  agent.sendRegistrationResponse(accept_data);
}

void AgentPool::reset() {
  for (const auto &agent : _pipeline_agents) {
    sendRegistrationInvalid(*agent);
  }

  _pipeline_agents.clear();

  sendAllAgentsInvalid(_registered_agents);
}

}  // namespace metal
