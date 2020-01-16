#pragma once

namespace metal {

class RegisteredAgent;

class ClientError : public std::runtime_error {
 public:
  explicit ClientError(std::shared_ptr<RegisteredAgent> agent,
                       const std::string& message)
      : std::runtime_error(message), _agent(agent) {}
  std::shared_ptr<RegisteredAgent> agent() const { return _agent; }

 protected:
  std::shared_ptr<RegisteredAgent> _agent;
};

}  // namespace metal
