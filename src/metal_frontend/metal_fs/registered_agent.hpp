#pragma once

#include <metal_frontend/common/socket.hpp>
#include <metal_frontend/common/buffer.hpp>

namespace metal {

// TODO: Disallow copy, destroy socket and buffer objects

class RegisteredAgent {
 public:
  explicit RegisteredAgent(Socket socket) : input_buffer(std::nullopt), output_buffer(std::nullopt), socket(socket) {}

  uint pid;
  std::string operator_type;

  Socket socket;

  std::string cwd;
  std::string metal_mountpoint;
  std::vector<std::string> args;

  std::weak_ptr<RegisteredAgent> input_agent;
  uint input_agent_pid;
  int input_file;
  std::optional<Buffer> input_buffer;

  std::string internal_input_file;
  std::string internal_output_file;

  std::weak_ptr<RegisteredAgent> output_agent;
  int output_agent_pid;
  int output_file;
  std::optional<Buffer> output_buffer;
};

}  // namespace metal
