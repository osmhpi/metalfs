#pragma once

#include <memory>
#include <unordered_map>
#include "user_operator.hpp"

namespace metal {

class OperatorRegistry {
public:
    explicit OperatorRegistry(const std::string &search_path);

    const std::unordered_map<std::string, std::shared_ptr<UserOperator>> & operators() const { return _operators; }

protected:
    std::unordered_map<std::string, std::shared_ptr<UserOperator>> _operators;
};

} // namespace metal
