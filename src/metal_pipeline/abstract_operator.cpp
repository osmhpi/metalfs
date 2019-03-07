#include "pipeline_runner.hpp"
#include "abstract_operator.hpp"


namespace metal {

void AbstractOperator::configure(SnapAction &action) {
    // TODO: Delete?
}

void AbstractOperator::finalize(SnapAction &action) {
}

void AbstractOperator::setOption(std::string option, OperatorArgumentValue arg) {
    auto o = _optionDefinitions.find(option);
    if (o == _optionDefinitions.end())
        throw std::runtime_error("Unknown option");

    if (o->second.type() != (OptionType)arg.index()) {
        throw std::runtime_error("Invalid option");
    }

    _options.at(option) = std::move(arg);
}

} // namespace metal
