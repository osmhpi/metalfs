extern "C" {
#include <unistd.h>
}

#include <spdlog/spdlog.h>
#include <algorithm>
#include <iostream>

#include <snap_action_metal.h>
#include <metal-pipeline/common.hpp>
#include <metal-pipeline/pipeline_definition.hpp>
#include <metal-pipeline/snap_action.hpp>
#include <metal-pipeline/user_operator_specification.hpp>

namespace metal {

PipelineDefinition::PipelineDefinition(std::vector<UserOperator> userOperators)
    : _cached_switch_configuration(false) {
  std::vector<UserOperatorRuntimeContext> contexts;
  contexts.reserve(userOperators.size());

  for (auto &op : userOperators) {
    contexts.emplace_back(UserOperatorRuntimeContext(std::move(op)));
  }

  _operators = std::move(contexts);
}

PipelineDefinition::PipelineDefinition(
    std::vector<UserOperatorRuntimeContext> userOperators)
    : _operators(std::move(userOperators)),
      _cached_switch_configuration(false) {}

uint64_t PipelineDefinition::run(DataSource dataSource, DataSink dataSink,
                                 SnapAction &action) {
  for (auto &op : _operators) {
    op.configure(action);
  }

  uint64_t enable_mask = 0;

  for (const auto &op : _operators)
    if (op.needs_preparation()) {
      enable_mask |= (1u << op.userOperator().spec().internal_id());
    }

  if (enable_mask) {
    // At least one operator needs preparation.
    action.execute_job(fpga::JobType::RunOperators, nullptr, {}, {},
                       enable_mask);
  }

  if (!_cached_switch_configuration) configureSwitch(action, false);

  enable_mask = 0;
  for (auto &op : _operators) {
    op.set_is_prepared();
    enable_mask |= (1u << op.userOperator().spec().internal_id());
  }

  uint64_t output_size;
  spdlog::debug(
      "Running Pipeline, Read: (Address: {:x}, Size: {}, Type: {}), Write: "
      "(Address: {:x}, Size: {}, Type: {})",
      dataSource.address().addr, dataSource.address().size,
      (uint64_t)dataSource.address().type, dataSink.address().addr,
      dataSink.address().size, (uint64_t)dataSink.address().type);
  action.execute_job(fpga::JobType::RunOperators, nullptr, dataSource.address(),
                     dataSink.address(), enable_mask,
                     /* perfmon_enable = */ 1, &output_size);

  for (auto &op : _operators) {
    op.finalize(action);
  }

  return output_size;
}

void PipelineDefinition::configureSwitch(SnapAction &action, bool set_cached) {
  _cached_switch_configuration = set_cached;

  auto *job_struct =
      reinterpret_cast<uint32_t *>(action.allocateMemory(sizeof(uint32_t) * 8));
  const uint32_t disable = 0x80000000;
  for (int i = 0; i < 8; ++i) {
    job_struct[i] = htobe32(disable);
  }

  uint8_t previous_op_stream = IOStreamID;
  for (const auto &op : _operators) {
    // From the perspective of the Stream Switch:
    // Which Master port (output) should be
    // sourced from which Slave port (input)
    job_struct[op.userOperator().spec().internal_id()] =
        htobe32(previous_op_stream);
    previous_op_stream = op.userOperator().spec().internal_id();
  }
  job_struct[IOStreamID] = previous_op_stream;

  try {
    action.execute_job(fpga::JobType::ConfigureStreams,
                       reinterpret_cast<char *>(job_struct));
  } catch (std::exception &ex) {
    free(job_struct);
    throw ex;
  }

  free(job_struct);
}

}  // namespace metal
