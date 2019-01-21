#pragma once

#include <variant>
#include <vector>
#include <memory>

namespace metal {

//class OperatorArgument : public std::variant<int, bool, std::unique_ptr<std::vector<char>>> {
//
//public:
//    bool holds_bool() const { return std::holds_alternative<bool>(*this); }
////    bool holds_bool() const { return std::holds_alternative<bool>(*this); }
//
//};

using OperatorArgument = std::variant<int, bool, std::unique_ptr<std::vector<char>>>;

} // namespace metal