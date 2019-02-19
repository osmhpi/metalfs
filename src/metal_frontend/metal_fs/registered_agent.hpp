#pragma once

namespace metal {

struct RegisteredAgent {
  uint pid;
  std::string operator_type;

  int socket;

  std::string cwd;
  std::string metal_mountpoint;
  std::vector<std::string> args;

  std::weak_ptr<RegisteredAgent> input_agent;
  uint input_agent_pid;
  int input_file;
  char* input_buffer;

  std::string internal_input_file;
  std::string internal_output_file;

  std::weak_ptr<RegisteredAgent> output_agent;
  int output_agent_pid;
  int output_file;
  char* output_buffer;
};

}  // namespace metal
