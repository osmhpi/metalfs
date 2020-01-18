#pragma once

#include <memory>

#include <cxxopts.hpp>

#include <metal-driver-messages/buffer.hpp>
#include <metal-driver-messages/socket.hpp>

namespace metal {

// TODO: Disallow copy, destroy socket and buffer objects

class RegisteredAgent : public std::enable_shared_from_this<RegisteredAgent> {
 public:
  explicit RegisteredAgent(Socket socket)
      : input_buffer(std::nullopt),
        output_buffer(std::nullopt),
        socket(std::move(socket)) {}

  std::string resolvePath(std::string relativeOrAbsolutePath);
  cxxopts::ParseResult parseOptions(cxxopts::Options &options);

  void sendRegistrationResponse(RegistrationResponse &message);
  ProcessingRequest receiveProcessingRequest();
  void sendProcessingResponse(ProcessingResponse &message);

  uint pid{};
  std::string operator_type;

  std::string cwd;
  std::string metal_mountpoint;
  std::vector<std::string> args;

  uint input_agent_pid{};
  std::optional<Buffer> input_buffer;

  std::string internal_input_file;
  std::string internal_output_file;

  std::shared_ptr<RegisteredAgent> output_agent;
  uint output_agent_pid{};
  std::optional<Buffer> output_buffer;

  std::string error;

 protected:
  Socket socket;
};

}  // namespace metal
