#include <metal-pipeline/operator_runtime_context.hpp>

#include <cstring>

#include <spdlog/spdlog.h>

#include <metal-pipeline/pipeline_runner.hpp>
#include <metal-pipeline/snap_action.hpp>
#include <metal-pipeline/user_operator.hpp>
#include <metal-pipeline/user_operator_specification.hpp>

namespace metal {

UserOperatorRuntimeContext::UserOperatorRuntimeContext(UserOperator op) : _op(std::move(op)) {}

void UserOperatorRuntimeContext::configure(SnapAction &action) {
    // Allocate job struct memory aligned on a page boundary, hopefully 4KB is sufficient
    auto job_config = reinterpret_cast<char*>(action.allocateMemory(4096));
    auto *metadata = reinterpret_cast<uint32_t*>(job_config);

    for (const auto& option : _op.options()) {
        if (!option.second.has_value())
            continue;

        const auto &definition = _op.spec().optionDefinitions().at(option.first);

        metadata[0] = htobe32(definition.offset() / sizeof(uint32_t)); // offset
        metadata[2] = htobe32(_op.spec().internal_id());
        metadata[3] = htobe32(_op.spec().prepare_required()); // enables preparation mode for the operator, implying that we need a preparation run of the operator(s)
        const int configuration_data_offset = 4 * sizeof(uint32_t); // bytes

        switch (static_cast<OptionType>(option.second.value().index())) {
            case OptionType::Uint: {
                uint32_t value = std::get<uint32_t>(option.second.value());
                std::memcpy(job_config + configuration_data_offset, &value, sizeof(uint32_t));
                metadata[1] = htobe32(sizeof(uint32_t) / sizeof(uint32_t)); // length
                break;
            }
            case OptionType::Bool: {
                uint32_t value = std::get<bool>(option.second.value()) ? 1 : 0;
                std::memcpy(job_config + configuration_data_offset, &value, sizeof(uint32_t));
                metadata[1] = htobe32(1); // length
                break;
            }
            case OptionType::Buffer: {
                // *No* endianness conversions on buffers
                auto& buffer = *std::get<std::shared_ptr<std::vector<char>>>(option.second.value());
                std::memcpy(job_config + configuration_data_offset, buffer.data(), buffer.size());
                metadata[1] = htobe32(buffer.size() / sizeof(uint32_t)); // length
                break;
            }
            default: {
                break;
            }
        }

        _is_prepared = false;

        try {
            action.execute_job(fpga::JobType::ConfigureOperator, job_config);
        } catch (std::exception &ex) {
            // Something went wrong...
            spdlog::warn("Could not configure operator: {}", ex.what());
        }
    }

    free(job_config);
}

void UserOperatorRuntimeContext::finalize(SnapAction &action) {
    (void)action;
}

bool UserOperatorRuntimeContext::needs_preparation() const {
    return !_is_prepared && _op.spec().prepare_required();
}

} // namespace metal
