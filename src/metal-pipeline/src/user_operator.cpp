#include <metal-pipeline/user_operator.hpp>

extern "C" {
#include <unistd.h>
}


#include <stdexcept>
#include <iostream>
#include <snap_action_metal.h>
#include <spdlog/spdlog.h>

#include <metal-pipeline/snap_action.hpp>
#include <metal-pipeline/user_operator_specification.hpp>

namespace metal {

UserOperator::UserOperator(std::shared_ptr<const UserOperatorSpecification> spec) : _spec(std::move(spec)) {
    for (const auto & option : _spec->optionDefinitions()) {
        _options.insert(std::make_pair(option.first, std::nullopt));
    }
}

std::string UserOperator::id() const {
    return _spec->id();
}

std::string UserOperator::description() const {
    return _spec->description();
}

void UserOperator::setOption(std::string option, OperatorArgumentValue arg) {
    auto o = _spec->optionDefinitions().find(option);
    if (o == _spec->optionDefinitions().end())
        throw std::runtime_error("Unknown option");

    if (o->second.type() != static_cast<OptionType>(arg.index())) {
        throw std::runtime_error("Invalid option");
    }

    _options.at(option) = std::move(arg);
}

} // namespace metal
