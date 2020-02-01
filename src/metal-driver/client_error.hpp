#pragma once

namespace metal {

class OperatorAgent;

class ClientError : public std::runtime_error {
 public:
  explicit ClientError(std::shared_ptr<OperatorAgent> agent,
                       const std::string& message)
      : std::runtime_error(message), _agent(agent) {}
  std::shared_ptr<OperatorAgent> agent() const { return _agent; }

 protected:
  std::shared_ptr<OperatorAgent> _agent;
};

}  // namespace metal
