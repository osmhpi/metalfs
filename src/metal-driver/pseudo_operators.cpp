#include "pseudo_operators.hpp"

#include "operator_agent.hpp"

namespace metal {

bool DatagenOperator::isDatagenAgent(const OperatorAgent &agent) {
  return agent.operatorType() == id();
}

void DatagenOperator::validate(OperatorAgent &agent) {
  // This will throw if something is wrong
  parseOptions(agent);
}

bool DatagenOperator::isProfilingEnabled(OperatorAgent &agent) {
  auto optionValues = parseOptions(agent);
  return optionValues["profile"].as<bool>();
}

uint64_t DatagenOperator::datagenLength(OperatorAgent &agent) {
  auto optionValues = parseOptions(agent);
  return optionValues["length"].as<uint64_t>();
}

cxxopts::ParseResult DatagenOperator::parseOptions(OperatorAgent &agent) {
  auto options =
      cxxopts::Options(id(),
                       "Generate data on the FPGA for benchmarking operators.");

  options.add_option("", "h", "help", "Print help",
                     cxxopts::value<bool>()->default_value("false"), "");
  options.add_option("", "p", "profile", "Enable profiling",
                     cxxopts::value<bool>()->default_value("false"), "");
  options.add_option("", "l", "length",
                     "The amount of data to generate, in bytes",
                     cxxopts::value<uint64_t>()->default_value("4096"), "");

  return agent.parseOptions(options);
}

bool DevNullFile::isNullOutput(const OperatorAgent &agent) {
  return agent.internalOutputFile() == "$NULL";
}

}  // namespace metal
