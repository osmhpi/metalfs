#pragma once

extern "C" {
#include <jv.h>
}

#include <string>
#include <unordered_map>

#include "abstract_operator.hpp"
#include "operator_argument.hpp"

namespace metal {

class UserOperator : public AbstractOperator {

public:
    explicit UserOperator(std::string manifest_path);
    virtual ~UserOperator();

    virtual void parseArguments() override;
    virtual void configure() override;
    virtual void finalize() override;

    void setOption(std::string option, OperatorArgument arg);

    virtual std::string id() const override;

protected:
    jv manifest() const { return jv_copy(_manifest); }

    jv _manifest;
    std::unordered_map<std::string, OperatorArgument> _options;

};

} // namespace metal
