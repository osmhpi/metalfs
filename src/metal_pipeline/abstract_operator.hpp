#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "operator_argument.hpp"

namespace metal {

class SnapAction;

class AbstractOperator {
public:
    virtual void configure(SnapAction &action) = 0;
    virtual void finalize(SnapAction &action) = 0;

    virtual std::string id() const = 0;
    virtual uint8_t internal_id() const = 0;

    std::unordered_map<std::string, OperatorOptionDefinition> &optionDefinitions() { return _optionDefinitions; }

    void setOption(std::string option, OperatorArgumentValue arg);

protected:
    bool _profilingEnabled {false};

    std::unordered_map<std::string, std::optional<OperatorArgumentValue>> _options;
    std::unordered_map<std::string, OperatorOptionDefinition> _optionDefinitions;
};

} // namespace metal
