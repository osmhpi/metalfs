#include <unistd.h>
#include <metal_frontend/common/message.h>
#include <metal_frontend/messages/message_header.hpp>

#include "agent_pool.hpp"
#include "server.hpp"
#include "registered_agent.hpp"

namespace metal {


void AgentPool::register_agent(ClientHello &hello, int socket) {
  auto agent = std::make_shared<RegisteredAgent>();

  agent->socket = socket;
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

  // If the agent's input is not connected to another agent, it should
  // have provided a file that will be used for memory-mapped data exchange
  // except when we're writing to an internal file
  if (agent->input_agent_pid == 0 && agent->internal_input_file.empty()) {
    if (hello.input_buffer_filename().empty()) {
      // Should not happen
      close(socket);
      return;
    }

    // TODO: Same procedure for output buffer
    map_shared_buffer_for_reading(
            hello.input_buffer_filename(),
            &agent->input_file, &agent->input_buffer
    );
  }

  _registered_agents.emplace(agent);

  update_agent_wiring();

  _contains_valid_pipeline_cached = false;
}

bool AgentPool::contains_valid_pipeline() {
  if (_contains_valid_pipeline_cached)
    return true;

  // Looks for agents at the beginning of a pipeline
  // that have not been signaled yet

  auto pipelineStart = *std::find_if(_registered_agents.begin(), _registered_agents.end(), [](const std::shared_ptr<RegisteredAgent> & a) {
    return a->input_agent_pid == 0;
  });

  // Walk the pipeline
  std::shared_ptr<RegisteredAgent> pipeline_agent = pipelineStart;
  uint64_t pipeline_length = 0;
  while (pipeline_agent && pipeline_agent->output_agent_pid != 0) {
    pipeline_agent = pipeline_agent->output_agent.lock();
    ++pipeline_length;
  }

  if (pipeline_agent) {
    // We have found an agent with output_agent_pid == 0, meaning it's an external file or pipe
    // So we have found a complete pipeline!
    // Now, remove all agents that are part of the pipeline from registered_agents and instead
    // insert them into pipeline_agents
    pipeline_agent = pipelineStart;
    while (pipeline_agent) {
      _registered_agents.erase(pipeline_agent);
      _pipeline_agents.insert(pipeline_agent);

      if (pipeline_agent->output_agent_pid == 0)
        break;

      pipeline_agent = pipeline_agent->output_agent.lock();
    }
  }
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
  accept_data.set_error_msg("An invalid operator chain was specified.\n");
  accept_data.set_valid(false);

  MessageHeader header {message_type::SERVER_ACCEPT_AGENT, accept_data.ByteSize() };
  header.sendHeader(agent.socket);
  send(agent.socket, &accept_data, header.length(), 0);
}

void AgentPool::reset() {
  send_all_agents_invalid(_pipeline_agents);
  send_all_agents_invalid(_registered_agents);
}

} // namespace metal
