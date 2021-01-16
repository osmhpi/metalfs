#include <metal-pipeline/operator.hpp>

extern "C" {
#include <unistd.h>
}

#include <metal-pipeline/fpga_interface.hpp>
#include <spdlog/spdlog.h>
#include <iostream>
#include <stdexcept>

#include <metal-pipeline/operator_specification.hpp>

namespace metal {

Operator::Operator(
    std::shared_ptr<const OperatorSpecification> spec)
    : _spec(std::move(spec)) {
  for (const auto& option : _spec->optionDefinitions()) {
    _options.insert(std::make_pair(option.first, std::nullopt));
  }
}

std::string Operator::id() const { return _spec->id(); }

std::string Operator::description() const { return _spec->description(); }

void Operator::setOption(std::string option, OperatorArgumentValue arg) {
  auto o = _spec->optionDefinitions().find(option);
  if (o == _spec->optionDefinitions().end())
    throw std::runtime_error("Unknown option");

  if (o->second.type() != static_cast<OptionType>(arg.index())) {
    throw std::runtime_error("Invalid option");
  }

  _options.at(option) = std::move(arg);
}

}  // namespace metal
