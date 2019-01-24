#pragma once

extern "C" {
#include <jv.h>
}

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "abstract_operator.hpp"
#include "operator_argument.hpp"

namespace cxxopts { class Options; }

namespace metal {

class UserOperator : public AbstractOperator {

public:
    explicit UserOperator(std::string manifest_path);
    virtual ~UserOperator();

    void parseArguments(std::vector<std::string> args) override;
    void configure(SnapAction& action) override;
    void finalize(SnapAction& action) override;

    void setOption(std::string option, OperatorArgumentValue arg);

    std::string id() const override;
    uint8_t temp_stream_id() const override;
    uint8_t temp_enable_id() const override;
    std::string description() const;

protected:
    void initializeOptions();

    jv manifest() const { return jv_copy(_manifest); }

    jv _manifest;
    std::unordered_map<std::string, OperatorArgument> _options;
    std::unique_ptr<cxxopts::Options> _opts;
    std::unordered_map<std::string, OptionType> _optionTypes;
    std::unordered_map<std::string, size_t> _fileOptions;

};

} // namespace metal
