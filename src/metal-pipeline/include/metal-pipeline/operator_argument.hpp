#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <variant>
#include <vector>
#include <memory>

namespace metal {

using OperatorArgumentValue = std::variant<uint32_t, bool, std::shared_ptr<std::vector<char>>>;

// Order matches the order of types above
enum class OptionType : size_t {
    Uint,
    Bool,
    Buffer
};

class METAL_PIPELINE_API OperatorOptionDefinition {
public:
    OperatorOptionDefinition() : OperatorOptionDefinition(0, static_cast<OptionType>(0), "", "", "") {}
    OperatorOptionDefinition(uint64_t offset, OptionType type, const std::string &key, const std::string &shrt,
                             const std::string &description, const std::optional<size_t> bufferSize = std::nullopt) :
             _offset(offset), _type(type), _key(key), _short(shrt), _description(description), _bufferSize(bufferSize) {}

    uint64_t offset() const { return _offset; }
    OptionType type() const { return _type; }
    std::string key() const { return _key; }
    std::string shrt() const { return _short; }
    std::string description() const { return _description; }
    std::optional<size_t> bufferSize() const { return _bufferSize; }

protected:
    uint64_t _offset;
    OptionType _type;
    std::string _key;
    std::string _short;
    std::string _description;
    std::optional<size_t> _bufferSize;
};

class METAL_PIPELINE_API OperatorArgument {
public:
    explicit OperatorArgument(size_t offset = 0)
        : _value(std::nullopt), _offset(offset) {};

    size_t offset() const { return _offset; }
    bool hasValue() const { return _value.has_value(); }
    const OperatorArgumentValue & value() const { return _value.value(); }
    void setValue(OperatorArgumentValue value) { _value = std::move(value); }

protected:
    std::optional<OperatorArgumentValue> _value;
    size_t _offset;
};



} // namespace metal
