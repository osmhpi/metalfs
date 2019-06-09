#pragma once

#include <memory>
#include <unordered_map>
#include "user_operator.hpp"

namespace metal {

class OperatorRegistry {
public:
    explicit OperatorRegistry(const std::string &search_path);

    void add_operator(std::string id, std::shared_ptr<AbstractOperator> op) { _operators.emplace(std::make_pair(std::move(id), std::move(op))); }
    const std::unordered_map<std::string, std::shared_ptr<AbstractOperator>> & operators() const { return _operators; }

protected:
    std::unordered_map<std::string, std::shared_ptr<AbstractOperator>> _operators;
};

} // namespace metal
