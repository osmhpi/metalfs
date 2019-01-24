#pragma once

#include <variant>
#include <vector>
#include <memory>

namespace metal {

using OperatorArgumentValue = std::variant<int, bool, std::shared_ptr<std::vector<char>>>;

// Order matches the order of types above
enum class OptionType : size_t {
    INT,
    BOOL,
    BUFFER
};

class OperatorArgument {
public:
    explicit OperatorArgument(size_t offset = 0)
        : _offset(offset), _value(std::nullopt) {};

    size_t offset() const { return _offset; }
    bool hasValue() const { return _value.has_value(); }
    const OperatorArgumentValue & value() const { return _value.value(); }
    void setValue(OperatorArgumentValue value) { _value = std::move(value); }

protected:
    std::optional<OperatorArgumentValue> _value;
    size_t _offset;
};



} // namespace metal