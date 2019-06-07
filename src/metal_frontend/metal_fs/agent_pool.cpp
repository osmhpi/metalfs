#include <unistd.h>
#include <metal_frontend/messages/message_header.hpp>
#include <algorithm>
#include <metal_frontend/messages/buffer.hpp>

#include "agent_pool.hpp"
#include "server.hpp"
#include "registered_agent.hpp"

namespace metal {

void AgentPool::register_agent(ClientHello &hello, int socket) {
  auto agent = std::make_shared<RegisteredAgent>(Socket(socket));

  agent->pid = hello.pid();
  agent->operator_type = hello.operator_type();
  agent->input_agent_pid = hello.input_pid();
  agent->output_agent_pid = hello.output_pid();
  agent->args.reserve(hello.args().size());
  for (const auto& arg : hello.args())
    agent->args.emplace_back(arg);

  agent->cwd = hello.cwd();
  agent->metal_mountpoint = hello.metal_mountpoint();

  if (hello.has_metal_input_filename())
    agent->internal_input_file = hello.metal_input_filename();
  if (hello.has_metal_output_filename())
    agent->internal_output_file = hello.metal_output_filename();

  _registered_agents.emplace(agent);

  _contains_valid_pipeline_cached = false;
}

bool AgentPool::contains_valid_pipeline() {
  if (_contains_valid_pipeline_cached)
    return true;

  _contains_valid_pipeline_cached = false;

  auto pipelineStart = *std::find_if(_registered_agents.begin(), _registered_agents.end(), [](const std::shared_ptr<RegisteredAgent> & a) {
    return a->input_agent_pid == 0;
  });

  if (!pipelineStart)
    return false;

  // Walk the pipeline until we find an agent with output_pid == 0
  std::shared_ptr<RegisteredAgent> pipeline_agent = pipelineStart;
  std::vector<std::shared_ptr<RegisteredAgent>> pipeline_agents;
  pipeline_agents.emplace_back(pipeline_agent);

  while (pipeline_agent && pipeline_agent->output_agent_pid != 0) {
    pipeline_agents.emplace_back(pipeline_agent);
    pipeline_agent = pipeline_agent->output_agent.lock();
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

void AgentPool::release_unused_agents() {
  send_all_agents_invalid(_registered_agents);
}

void AgentPool::send_all_agents_invalid(std::unordered_set<std::shared_ptr<RegisteredAgent>> &agents) {
  for (const auto &agent : agents) {
    send_agent_invalid(*agent);
  }

  agents.clear();
}

void AgentPool::send_agent_invalid(RegisteredAgent &agent) {
  metal::ServerAcceptAgent accept_data;

  if (!agent.error.empty()) {
      accept_data.set_error_msg(agent.error);
  } else {
      accept_data.set_error_msg("An invalid operator chain was specified.\n");
  }

  accept_data.set_valid(false);

  agent.socket.send_message<message_type::SERVER_ACCEPT_AGENT>(accept_data);
}

void AgentPool::reset() {
  for (const auto &agent : _pipeline_agents) {
    send_agent_invalid(*agent);
  }

  _pipeline_agents.clear();

  send_all_agents_invalid(_registered_agents);
}

} // namespace metal
