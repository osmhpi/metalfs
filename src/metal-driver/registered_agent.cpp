#include "registered_agent.hpp"

#include "client_error.hpp"

namespace metal {

std::string RegisteredAgent::resolvePath(std::string relativeOrAbsolutePath) {
  if (!relativeOrAbsolutePath.size()) {
    return cwd;
  }

  if (relativeOrAbsolutePath[0] == '/') {
    // is absolute
    return relativeOrAbsolutePath;
  }

  return cwd + "/" + relativeOrAbsolutePath;
}

cxxopts::ParseResult RegisteredAgent::parseOptions(cxxopts::Options& options) {
  // Contain some C++ / C interop ugliness inside here...

  std::vector<char *> argsRaw;
  for (const auto &arg : args)
    argsRaw.emplace_back(const_cast<char *>(arg.c_str()));

  int argc = (int)args.size();
  char **argv = argsRaw.data();
  auto parseResult = options.parse(argc, argv);

  if (parseResult["help"].as<bool>()) {
    // Not really an exception, but the fastest way how we can exit the
    // processing flow
    throw ClientError(shared_from_this(), options.help());
  }
}

}  // namespace metal
