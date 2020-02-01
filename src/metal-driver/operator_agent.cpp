#include "operator_agent.hpp"

#include <limits.h>

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
  auto request = _socket.receiveMessage<MessageType::RegistrationRequest>();

  auto logInput = request.has_metal_input_filename()
                      ? request.metal_input_filename()
                      : ("pid/" + std::to_string(request.input_pid()));
  auto logOutput = request.has_metal_output_filename()
                       ? request.metal_output_filename()
                       : ("pid/" + std::to_string(request.output_pid()));
  spdlog::trace("RegistrationRequest(operator={}, input={}, output={})",
                request.operator_type(), logInput, logOutput);

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

void OperatorAgent::setInputFile(const std::string &input) {
  auto absPath = resolvePath(input);

  char actualpath[PATH_MAX + 1];
  char *ptr;
  ptr = realpath(absPath.c_str(), actualpath);

  if (ptr == nullptr) {
    throw ClientError(shared_from_this(), "Could not find input file.");
  }

  std::string realPath(ptr);
  auto filesPrefix = _metalMountpoint + "/files/";
  if (realPath.rfind(filesPrefix, 0) == 0) {
    if (_internalInputFile.empty()) {
      // TODO: Before we go ahead and read the file for the user, we should
      // check access permissions
      _internalInputFile = "/" + realPath.substr(filesPrefix.size());
    }
  } else {
    _agentLoadFile = realPath;
  }
}

void OperatorAgent::sendRegistrationResponse(RegistrationResponse &message) {
  spdlog::trace(
      "RegistrationResponse(valid={}, mapInputBuffer={}, mapOutputBuffer={}, "
      "loadInputFile={})",
      message.valid(), message.has_input_buffer_filename(),
      message.has_output_buffer_filename(), message.agent_read_filename());
  _socket.sendMessage<MessageType::RegistrationResponse>(message);
}

ProcessingRequest OperatorAgent::receiveProcessingRequest() {
  auto request = _socket.receiveMessage<MessageType::ProcessingRequest>();
  spdlog::trace("ProcessingRequest(size={}, eof={})", request.size(),
                request.eof());
  return request;
}

void OperatorAgent::sendProcessingResponse(ProcessingResponse &message) {
  spdlog::trace("ProcessingResponse(size={}, eof={})", message.size(),
                message.eof());
  _socket.sendMessage<MessageType::ProcessingResponse>(message);
}

}  // namespace metal
