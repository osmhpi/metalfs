#include <utility>

#pragma once

#include <string>
#include <unordered_set>
#include <Messages.pb.h>
#include <metal_pipeline/operator_registry.hpp>
#include "../../../third_party/cxxopts/include/cxxopts.hpp"

void* start_socket(void* args);

namespace metal {

class MessageHeader;

struct RegisteredAgent {
    uint pid;
    std::string operator_type;

    int socket;

    std::string cwd;
    std::string metal_mountpoint;
    std::vector<std::string> args;

    std::weak_ptr<RegisteredAgent> input_agent;
    uint input_agent_pid;
    int input_file;
    char* input_buffer;

    std::string internal_input_file;
    std::string internal_output_file;

    std::weak_ptr<RegisteredAgent> output_agent;
    int output_agent_pid;
    int output_file;
    char* output_buffer;
};


class Server {
public:

    explicit Server(std::string socketFileName);;

    static void start(void* args);
protected:
    void startInternal();


    void process_request(int connfd);
    void register_agent(ClientHello &request, int connfd);
    void derive_execution_plan();
    void send_all_agents_invalid(std::unordered_set<std::shared_ptr<RegisteredAgent>> &agents);
    void send_agent_invalid(RegisteredAgent &agent);

    template<typename T>
    T receive_message(int connfd, MessageHeader &header);

    std::unordered_set<std::shared_ptr<RegisteredAgent>> _registered_agents;
    std::unordered_set<std::shared_ptr<RegisteredAgent>> _pipeline_agents;
    std::string _socketFileName;
    OperatorRegistry _registry;
    std::unordered_map<std::string, cxxopts::Options> _operatorOptions;

    void setOperatorOptionsFromInvocationData(std::shared_ptr<AbstractOperator> op,
                                              std::shared_ptr<RegisteredAgent> agent);
    std::string resolvePath(std::string relative_or_absolue_path, std::string working_dir);

    cxxopts::Options buildOperatorOptions(std::shared_ptr<UserOperator> shared_ptr);
};
} // namespace metal

