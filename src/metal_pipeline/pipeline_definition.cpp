extern "C" {
#include <unistd.h>
#include <snap_hls_if.h>
}

#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>

#include <metal_fpga/hw/hls/include/snap_action_metal.h>
#include "pipeline_definition.hpp"
#include "snap_action.hpp"

namespace metal {

PipelineDefinition::PipelineDefinition(std::vector<std::shared_ptr<AbstractOperator>> operators)
    : _operators(std::move(operators)), _cached_switch_configuration(false) {
    if (_operators.size() < 2) {
        throw std::runtime_error("Pipeline must contain at least data source and data sink operators.");
    }

    _dataSource = std::dynamic_pointer_cast<DataSource>(_operators.front());
    if (_dataSource == nullptr) {
        throw std::runtime_error("Pipeline does not start with a data source");
    }

    _dataSink = std::dynamic_pointer_cast<DataSink>(_operators.back());
    if (_dataSink == nullptr) {
        throw std::runtime_error("Pipeline does not end with a data sink");
    }
}

uint64_t PipelineDefinition::run(SnapAction &action) {
    for (const auto &op : _operators)
        op->configure(action);

    uint64_t enable_mask = 0;

    for (const auto &op : _operators)
        if (op->needs_preparation()) enable_mask |= (1u << op->internal_id());

    if (enable_mask) {
        // At least one operator needs preparation.
        action.execute_job(fpga::JobType::RunOperators, nullptr, {}, {}, enable_mask);
    }

    if (!_cached_switch_configuration)
        configureSwitch(action, false);

    enable_mask = 0;
    for (const auto &op : _operators) {
        op->set_is_prepared();
        enable_mask |= (1u << op->internal_id());
    }

    uint64_t output_size;
    spdlog::debug("Running Pipeline, Read: (Address: {:x}, Size: {}, Type: {}), Write: (Address: {:x}, Size: {}, Type: {})",
        _dataSource->address().addr, _dataSource->address().size, (uint64_t)_dataSource->address().type,
        _dataSink->address().addr, _dataSink->address().size, (uint64_t)_dataSink->address().type);
    action.execute_job(fpga::JobType::RunOperators, nullptr, _dataSource->address(), _dataSink->address(), enable_mask, /* perfmon_enable = */ 1, &output_size);

    for (const auto &op : _operators)
        op->finalize(action);

    return output_size;
}

void PipelineDefinition::configureSwitch(SnapAction &action, bool set_cached) {
    _cached_switch_configuration = set_cached;

    auto *job_struct = reinterpret_cast<uint32_t*>(snap_malloc(sizeof(uint32_t) * 8));
    const uint32_t disable = 0x80000000;
    for (int i = 0; i < 8; ++i)
        job_struct[i] = htobe32(disable);

    uint8_t previous_op_stream = !_operators.empty() ? _operators[0]->internal_id() : 0;
    for (const auto &op : _operators) {
        // From the perspective of the Stream Switch:
        // Which Master port (output) should be
        // sourced from which Slave port (input)
        job_struct[op->internal_id()] = htobe32(previous_op_stream);
        previous_op_stream = op->internal_id();
    }

    try {
        action.execute_job(fpga::JobType::ConfigureStreams, reinterpret_cast<char *>(job_struct));
    } catch (std::exception &ex) {
        free(job_struct);
        throw ex;
    }

    free(job_struct);
}

} // namespace metal
