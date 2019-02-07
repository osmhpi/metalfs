extern "C" {
#include <unistd.h>
#include <snap_hls_if.h>
}
#include <metal_fpga/hw/hls/include/action_metalfpga.h>
#include <iostream>
#include "pipeline_definition.hpp"
#include "snap_action.hpp"

namespace metal {

void PipelineDefinition::run(SnapAction &action) {

    configureSwitch(action);

    for (const auto &op : _operators)
        op->configure(action);

    action.execute_job(MTL_JOB_RUN_OPERATORS, nullptr);

    for (const auto &op : _operators)
        op->finalize(action);
}

void PipelineDefinition::configureSwitch(SnapAction &action) const {
    uint64_t enable_mask = 0;
    for (const auto &op : _operators)
        enable_mask |= (1 << op->temp_enable_id());

    auto *job_struct_enable = (uint64_t*)snap_malloc(sizeof(uint32_t) * 10);
    *job_struct_enable = htobe64(enable_mask);

    auto *job_struct = (uint32_t*)(job_struct_enable + 1);
    const uint32_t disable = 0x80000000;
    for (int i = 0; i < 8; ++i)
        job_struct[i] = htobe32(disable);

    uint8_t previous_op_stream = _operators.size() ? _operators[0]->temp_stream_id() : 0;
    for (const auto &op : _operators) {
        // From the perspective of the Stream Switch:
        // Which Master port (output) should be
        // sourced from which Slave port (input)
        job_struct[op->temp_stream_id()] = htobe32(previous_op_stream);
        previous_op_stream = op->temp_stream_id();
    }

    try {
        action.execute_job(MTL_JOB_CONFIGURE_STREAMS, reinterpret_cast<char *>(job_struct_enable));
    } catch (std::exception &ex) {
        free(job_struct_enable);
        throw ex;
    }

    free(job_struct_enable);
}

} // namespace metal