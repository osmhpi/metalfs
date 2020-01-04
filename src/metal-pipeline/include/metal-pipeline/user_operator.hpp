#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "operator_argument.hpp"

namespace metal {

class UserOperatorSpecification;

class METAL_PIPELINE_API UserOperator {
    std::shared_ptr<const UserOperatorSpecification> _spec;

public:
    explicit UserOperator(std::shared_ptr<const UserOperatorSpecification> spec);
    UserOperator(UserOperator &&other) : _spec(std::move(other._spec)), _options(std::move(other._options)) {};
    std::string id() const;
    std::string description() const;

    const std::unordered_map<std::string, std::optional<OperatorArgumentValue>> options() const { return _options; }

    const UserOperatorSpecification &spec() const { return *_spec; }

    void setOption(std::string option, OperatorArgumentValue arg);

protected:
    std::unordered_map<std::string, std::optional<OperatorArgumentValue>> _options;

};

} // namespace metal
