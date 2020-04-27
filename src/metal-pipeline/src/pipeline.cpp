extern "C" {
#include <unistd.h>
}

#include <spdlog/spdlog.h>
#include <algorithm>
#include <iostream>

#include <metal-pipeline/fpga_interface.hpp>
#include <metal-pipeline/common.hpp>
#include <metal-pipeline/operator_specification.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

Pipeline::Pipeline(std::vector<Operator> userOperators)
    : _cachedSwitchConfiguration(false) {
  std::vector<OperatorContext> contexts;
  contexts.reserve(userOperators.size());

  for (auto &op : userOperators) {
    contexts.emplace_back(OperatorContext(std::move(op)));
  }

  _operators = std::move(contexts);
}

Pipeline::Pipeline(std::vector<OperatorContext> userOperators)
    : _operators(std::move(userOperators)),
      _cachedSwitchConfiguration(false) {}

uint64_t Pipeline::run(DataSource dataSource, DataSink dataSink,
                       SnapAction &action) {
  for (auto &op : _operators) {
    op.configure(action);
  }

  uint64_t enable_mask = 0;

  for (const auto &op : _operators)
    if (op.needs_preparation()) {
      enable_mask |= (1u << op.userOperator().spec().streamID());
    }

  if (enable_mask) {
    // At least one operator needs preparation.
    action.executeJob(fpga::JobType::RunOperators, nullptr, {}, {},
                       enable_mask);
  }

  if (!_cachedSwitchConfiguration) configureSwitch(action, false);

  enable_mask = 1;  // This time, enable the I/O subsystem
  for (auto &op : _operators) {
    op.set_is_prepared();
    enable_mask |= (1u << op.userOperator().spec().streamID());
  }

  spdlog::debug("Running Pipeline");

  auto sourceAddress = dataSource.address(),
       destinationAddress = dataSink.address();

  spdlog::debug("  Read:  (Address: 0x{:016x}   Size: {:>8d}   Type: {:<8}   Map: {:<12})",
                sourceAddress.addr, sourceAddress.size,
                SnapAction::addressTypeToString(sourceAddress.type),
                SnapAction::mapTypeToString(sourceAddress.map));
  spdlog::debug("  Write: (Address: 0x{:016x}   Size: {:>8d}   Type: {:<8}   Map: {:<12})",
                destinationAddress.addr, destinationAddress.size,
                SnapAction::addressTypeToString(destinationAddress.type),
                SnapAction::mapTypeToString(destinationAddress.map));

  uint64_t output_size;
  action.executeJob(fpga::JobType::RunOperators, nullptr, sourceAddress,
                     destinationAddress, enable_mask, /* perfmon_enable = */ 1,
                     &output_size);

  spdlog::debug("Result size: {}", output_size);

  for (auto &op : _operators) {
    op.finalize(action);
  }

  return output_size;
}

void Pipeline::configureSwitch(SnapAction &action, bool set_cached) {
  _cachedSwitchConfiguration = set_cached;

  auto *job_struct =
      reinterpret_cast<uint32_t *>(action.allocateMemory(sizeof(uint32_t) * 8));
  const uint32_t disable = 0x80000000;
  for (int i = 0; i < 8; ++i) {
    job_struct[i] = htobe32(disable);
  }

  uint8_t previousStream = IOStreamID;
  for (const auto &op : _operators) {
    // From the perspective of the Stream Switch:
    // Which Master port (output) should be
    // sourced from which Slave port (input)
    auto currentStream = op.userOperator().spec().streamID();
    job_struct[currentStream] = htobe32(previousStream);
    previousStream = currentStream;
  }
  job_struct[IOStreamID] = htobe32(previousStream);

  try {
    action.executeJob(fpga::JobType::ConfigureStreams,
                       reinterpret_cast<char *>(job_struct));
  } catch (std::exception &ex) {
    free(job_struct);
    throw ex;
  }

  free(job_struct);
}

}  // namespace metal
