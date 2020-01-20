#include "operator_agent.hpp"

#include <spdlog/spdlog.h>

#include "client_error.hpp"

namespace metal {

OperatorAgent::OperatorAgent(Socket socket)
    : _args(),
      _inputBuffer(std::nullopt),
      _internalInputFile(),
      _internalOutputFile(),
      _outputAgent(nullptr),
      _outputBuffer(std::nullopt),
      _error(),
      _terminated(false),
      _socket(std::move(socket)) {
  auto request = _socket.receiveMessage<message_type::RegistrationRequest>();
  spdlog::trace("RegistrationRequest(operator={})", request.operator_type());

  _pid = request.pid();
  _operatorType = request.operator_type();
  _inputAgentPid = request.input_pid();
  _outputAgentPid = request.output_pid();
  _args.reserve(request.args().size());
  for (const auto &arg : request.args()) {
    _args.emplace_back(arg);
  }

  _cwd = request.cwd();
  _metalMountpoint = request.metal_mountpoint();

  if (request.has_metal_input_filename())
    _internalInputFile = request.metal_input_filename();
  if (request.has_metal_output_filename())
    _internalOutputFile = request.metal_output_filename();
}

std::string OperatorAgent::resolvePath(std::string relativeOrAbsolutePath) {
  if (!relativeOrAbsolutePath.size()) {
    return _cwd;
  }

  if (relativeOrAbsolutePath[0] == '/') {
    // is absolute
    return relativeOrAbsolutePath;
  }

  return _cwd + "/" + relativeOrAbsolutePath;
}

cxxopts::ParseResult OperatorAgent::parseOptions(cxxopts::Options &options) {
  // Contain some C++ / C interop ugliness inside here...

  std::vector<char *> argsRaw;
  for (const auto &arg : _args)
    argsRaw.emplace_back(const_cast<char *>(arg.c_str()));

  int argc = (int)_args.size();
  char **argv = argsRaw.data();
  auto parseResult = options.parse(argc, argv);

  if (parseResult["help"].as<bool>()) {
    // Not really an exception, but the fastest way how we can exit the
    // processing flow
    throw ClientError(shared_from_this(), options.help());
  }

  return parseResult;
}

void OperatorAgent::createInputBuffer() {
  _inputBuffer = Buffer::createTempFileForSharedBuffer(false);
}

void OperatorAgent::createOutputBuffer() {
  _outputBuffer = Buffer::createTempFileForSharedBuffer(true);
}

void OperatorAgent::sendRegistrationResponse(RegistrationResponse &message) {
  spdlog::trace(
      "RegistrationResponse(valid={}, input_filename={}, output_filename={})",
      message.valid(), message.has_input_buffer_filename(),
      message.has_output_buffer_filename());
  _socket.send_message<message_type::RegistrationResponse>(message);
}

ProcessingRequest OperatorAgent::receiveProcessingRequest() {
  auto request = _socket.receiveMessage<message_type::ProcessingRequest>();
  spdlog::trace("ProcessingRequest(size={}, eof={})", request.size(),
                request.eof());
  return request;
}

void OperatorAgent::sendProcessingResponse(ProcessingResponse &message) {
  spdlog::trace("ProcessingResponse(size={}, eof={})", message.size(),
                message.eof());
  _socket.send_message<message_type::ProcessingResponse>(message);
}

}  // namespace metal
