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
  static void setInputFile(OperatorAgent &agent);

 protected:
  static cxxopts::ParseResult parseOptions(OperatorAgent &agent);
};

class MetalCatOperator {
 public:
  static std::string id() { return "metal_cat"; }
  static bool isMetalCatAgent(const OperatorAgent &agent);
  static void validate(OperatorAgent &agent);
  static bool isProfilingEnabled(OperatorAgent &agent);
  static void setInputFile(OperatorAgent &agent);

 protected:
  static cxxopts::ParseResult parseOptions(OperatorAgent &agent);
};

class DevNullFile {
 public:
  static bool isNullOutput(const OperatorAgent &agent);
};

}  // namespace metal
