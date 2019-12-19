#include <metal-pipeline/pipeline_runner.hpp>
#include <metal-pipeline/abstract_operator.hpp>


namespace metal {

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
