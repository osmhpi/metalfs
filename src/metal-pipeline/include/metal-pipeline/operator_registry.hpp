#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <unordered_map>
#include "user_operator.hpp"

namespace metal {

class UserOperatorSpecification;

class METAL_PIPELINE_API OperatorRegistry {
public:
    explicit OperatorRegistry(const std::string &image_json);
    UserOperator createUserOperator(std::string id);

    void add_operator(std::string id, std::shared_ptr<const UserOperatorSpecification> op) { _operators.emplace(std::make_pair(std::move(id), std::move(op))); }
    size_t size() const { return _operators.size(); }

protected:
    std::unordered_map<std::string, std::shared_ptr<const UserOperatorSpecification>> _operators;
};

} // namespace metal
