#pragma once

#include <memory>

#include <cxxopts.hpp>

#include <metal-driver-messages/buffer.hpp>
#include <metal-driver-messages/socket.hpp>

namespace metal {

class OperatorAgent : public std::enable_shared_from_this<OperatorAgent> {
 public:
  explicit OperatorAgent(Socket socket);

  std::string resolvePath(std::string relativeOrAbsolutePath);
  cxxopts::ParseResult parseOptions(cxxopts::Options &options);

  bool isOutputConnectedTo(const OperatorAgent &other) {
    return other._pid == _outputAgentPid;
  }
  std::shared_ptr<OperatorAgent> outputAgent() { return _outputAgent; }
  void setOutputAgent(std::shared_ptr<OperatorAgent> outputAgent) {
    _outputAgent = outputAgent;
  }
  std::optional<Buffer> &inputBuffer() { return _inputBuffer; }
  std::optional<Buffer> &outputBuffer() { return _outputBuffer; }
  bool isInputAgent() { return _inputAgentPid == 0; }
  bool isOutputAgent() { return _outputAgentPid == 0; }
  const std::string &error() const { return _error; }
  void setError(const std::string &error) { _error = error; }
  const std::string &internalInputFile() const { return _internalInputFile; }
  const std::string &internalOutputFile() const { return _internalOutputFile; }
  const std::string &operatorType() const { return _operatorType; }
  bool terminated() const { return _terminated; }
  void setTerminated() { _terminated = true; }

  void createInputBuffer();
  void createOutputBuffer();

  void sendRegistrationResponse(RegistrationResponse &message);
  ProcessingRequest receiveProcessingRequest();
  void sendProcessingResponse(ProcessingResponse &message);

 protected:
  uint _pid;
  std::string _operatorType;

  std::string _cwd;
  std::string _metalMountpoint;
  std::vector<std::string> _args;

  uint _inputAgentPid;
  std::optional<Buffer> _inputBuffer;

  std::string _internalInputFile;
  std::string _internalOutputFile;

  std::shared_ptr<OperatorAgent> _outputAgent;
  uint _outputAgentPid;
  std::optional<Buffer> _outputBuffer;

  std::string _error;
  bool _terminated;

  Socket _socket;
};

}  // namespace metal
