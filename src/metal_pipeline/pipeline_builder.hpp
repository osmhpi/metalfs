#pragma once

#include <memory>
#include <vector>

namespace metal {

class AbstractOperator;

struct OperatorInvocation {
    std::string operator_name;
    std::vector<std::string> args;
    std::string working_dir;

    int pid;
    int input_pid;
    int output_pid;

    std::string input_file;
    std::string output_file;
};

class PipelineBuilder {
public:
    void add_operator(OperatorInvocation invocation);

    bool is_valid() const;
    std::vector<std::unique_ptr<AbstractOperator>> build() const;

protected:
//    std::unordered_map<std::string,

};

} // namespace metal
