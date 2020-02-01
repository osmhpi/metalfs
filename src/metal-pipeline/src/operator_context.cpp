#include <metal-pipeline/operator_context.hpp>

#include <cstring>

#include <spdlog/spdlog.h>

#include <metal-pipeline/snap_action.hpp>
#include <metal-pipeline/operator.hpp>
#include <metal-pipeline/operator_specification.hpp>

namespace metal {

OperatorContext::OperatorContext(Operator op)
    : _op(std::move(op)),
      _is_prepared(false),
      _profilingEnabled(false),
      _profilingResults() {}

void OperatorContext::configure(SnapAction &action) {
  // Allocate job struct memory aligned on a page boundary, hopefully 4KB is
  // sufficient
  auto job_config = reinterpret_cast<char *>(action.allocateMemory(4096));
  auto *metadata = reinterpret_cast<uint32_t *>(job_config);

  for (const auto &option : _op.options()) {
    if (!option.second.has_value()) continue;

    const auto &definition = _op.spec().optionDefinitions().at(option.first);

    metadata[0] = htobe32(definition.offset() / sizeof(uint32_t));  // offset
    metadata[2] = htobe32(_op.spec().streamID());
    metadata[3] = htobe32(
        _op.spec().prepareRequired());  // enables preparation mode for the
                                         // operator, implying that we need a
                                         // preparation run of the operator(s)
    const int configuration_data_offset = 4 * sizeof(uint32_t);  // bytes

    switch (static_cast<OptionType>(option.second.value().index())) {
      case OptionType::Uint: {
        uint32_t value = std::get<uint32_t>(option.second.value());
        std::memcpy(job_config + configuration_data_offset, &value,
                    sizeof(uint32_t));
        metadata[1] = htobe32(sizeof(uint32_t) / sizeof(uint32_t));  // length
        break;
      }
      case OptionType::Bool: {
        uint32_t value = std::get<bool>(option.second.value()) ? 1 : 0;
        std::memcpy(job_config + configuration_data_offset, &value,
                    sizeof(uint32_t));
        metadata[1] = htobe32(1);  // length
        break;
      }
      case OptionType::Buffer: {
        // *No* endianness conversions on buffers
        auto &buffer = *std::get<std::shared_ptr<std::vector<char>>>(
            option.second.value());
        std::memcpy(job_config + configuration_data_offset, buffer.data(),
                    buffer.size());
        metadata[1] = htobe32(buffer.size() / sizeof(uint32_t));  // length
        break;
      }
      default: { break; }
    }

    _is_prepared = false;

    try {
      action.executeJob(fpga::JobType::ConfigureOperator, job_config);
    } catch (std::exception &ex) {
      // Something went wrong...
      spdlog::warn("Could not configure operator: {}", ex.what());
    }
  }

  free(job_config);
}

void OperatorContext::finalize(SnapAction &action) { (void)action; }

bool OperatorContext::needs_preparation() const {
  return !_is_prepared && _op.spec().prepareRequired();
}

}  // namespace metal
