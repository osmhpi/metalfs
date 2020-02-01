#pragma once

#include <stdint.h>

#include <cxxopts.hpp>

namespace metal {

class OperatorAgent;

class DatagenOperator {
 public:
  static std::string id() { return "datagen"; }
  static bool isDatagenAgent(const OperatorAgent &agent);
  static void validate(OperatorAgent &agent);
  static bool isProfilingEnabled(OperatorAgent &agent);
  static uint64_t datagenLength(OperatorAgent &agent);

 protected:
  static cxxopts::ParseResult parseOptions(OperatorAgent &agent);
};

class DevNullFile {
 public:
  static bool isNullOutput(const OperatorAgent &agent);
};

}  // namespace metal
