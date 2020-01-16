#include <unistd.h>

#include <snap_hls_if.h>

#include <snap_action_metal.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <metal-pipeline/pipeline_runner.hpp>
#include <metal-pipeline/snap_action.hpp>
#include <sstream>

namespace metal {

std::string SnapPipelineRunner::readImageInfo(int card) {
  SnapAction action(fpga::ActionType, card);
  uint64_t json_len = 0;
  auto json = snap_malloc(4096);
  try {
    action.execute_job(fpga::JobType::ReadImageInfo, json, {}, {}, 0, 0,
                       &json_len);
  } catch (std::exception &ex) {
    free(json);
    throw ex;
  }

  std::string result(reinterpret_cast<const char *>(json), json_len);
  free(json);
  return result;
}

uint64_t SnapPipelineRunner::run(bool last) {
  SnapAction action = SnapAction(fpga::ActionType, _card);

  pre_run(action, !_initialized);
  _initialized = true;

  uint64_t output_size = _pipeline->run(action);

  post_run(action, last);

  return output_size;
}

void SnapPipelineRunner::pre_run(SnapAction &action, bool initialize) {
  if (initialize) {
    _pipeline->configureSwitch(action, true);
  }
}

void ProfilingPipelineRunner::pre_run(SnapAction &action, bool initialize) {
  if (_op != nullptr) {
    if (initialize) {
      auto operatorPosition = std::find(_pipeline->operators().begin(),
                                        _pipeline->operators().end(), _op);

      if (operatorPosition == _pipeline->operators().cend())
        throw std::runtime_error(
            "Operator to be profiled must be part of pipeline");

      auto *job_struct =
          reinterpret_cast<uint64_t *>(snap_malloc(sizeof(uint64_t) * 2));

      if (operatorPosition == _pipeline->operators().begin()) {
        // If the operator is the data source, we profile its output stream
        // twice
        ++operatorPosition;
        job_struct[0] = htobe64((*operatorPosition)->internal_id());
        job_struct[1] = htobe64((*operatorPosition)->internal_id());
      } else {
        job_struct[0] = htobe64(_op->internal_id());
        // If the operator is the data sink, we profile its input stream twice
        ++operatorPosition;
        if (operatorPosition == _pipeline->operators().end()) {
          job_struct[1] = htobe64(_op->internal_id());
        } else {
          job_struct[1] = htobe64((*operatorPosition)->internal_id());
        }
      }

      try {
        action.execute_job(fpga::JobType::ConfigurePerfmon,
                           reinterpret_cast<char *>(job_struct));
      } catch (std::exception &ex) {
        free(job_struct);
        throw ex;
      }

      free(job_struct);
    }

    action.execute_job(fpga::JobType::ResetPerfmon);
  }

  SnapPipelineRunner::pre_run(action, initialize);
}

void ProfilingPipelineRunner::post_run(SnapAction &action, bool finalize) {
  // Ignore finalize, obtain partial profiling results after each pipeline
  // invocation
  (void)finalize;

  if (_op != nullptr) {
    auto *results64 =
        reinterpret_cast<uint64_t *>(snap_malloc(sizeof(uint64_t) * 6));
    auto *results32 = reinterpret_cast<uint32_t *>(results64 + 1);

    try {
      action.execute_job(fpga::JobType::ReadPerfmonCounters,
                         reinterpret_cast<char *>(results64));
    } catch (std::exception &ex) {
      free(results64);
      throw ex;
    }

    _results.global_clock_counter += be64toh(results64[0]);

    _results.input_transfer_cycle_count += be32toh(results32[0]);
    _results.input_data_byte_count += be32toh(results32[2]);
    _results.input_slave_idle_count += be32toh(results32[3]);
    _results.input_master_idle_count += be32toh(results32[4]);

    _results.output_transfer_cycle_count += be32toh(results32[5]);
    _results.output_data_byte_count += be32toh(results32[7]);
    _results.output_slave_idle_count += be32toh(results32[8]);
    _results.output_master_idle_count += be32toh(results32[9]);

    free(results64);
  }
}

void ProfilingPipelineRunner::selectOperatorForProfiling(
    std::shared_ptr<AbstractOperator> op) {
  _op = std::move(op);
  requireReinitialization();
}

std::string ProfilingPipelineRunner::formatProfilingResults() {
  const double freq = 250;
  const double onehundred = 100;

  double input_transfer_cycle_percent = _results.input_transfer_cycle_count *
                                        onehundred /
                                        _results.global_clock_counter;
  double input_slave_idle_percent = _results.input_slave_idle_count *
                                    onehundred / _results.global_clock_counter;
  double input_master_idle_percent = _results.input_master_idle_count *
                                     onehundred / _results.global_clock_counter;
  double input_mbps = (_results.input_data_byte_count * freq) /
                      (double)_results.global_clock_counter;

  double output_transfer_cycle_percent = _results.output_transfer_cycle_count *
                                         onehundred /
                                         _results.global_clock_counter;
  double output_slave_idle_percent = _results.output_slave_idle_count *
                                     onehundred / _results.global_clock_counter;
  double output_master_idle_percent = _results.output_master_idle_count *
                                      onehundred /
                                      _results.global_clock_counter;
  double output_mbps = (_results.output_data_byte_count * freq) /
                       (double)_results.global_clock_counter;

  std::stringstream result;

  result << "STREAM\tBYTES TRANSFERRED  ACTIVE CYCLES  DATA WAIT      CONSUMER "
            "WAIT  TOTAL CYCLES  MB/s"
         << std::endl;

  result << string_format(
                "input\t%-17lu  %-9lu%3.0f%%  %-9lu%3.0f%%  %-9lu%3.0f%%  "
                "%-12lu  %-4.2f",
                _results.input_data_byte_count,
                _results.input_transfer_cycle_count,
                input_transfer_cycle_percent, _results.input_master_idle_count,
                input_master_idle_percent, _results.input_slave_idle_count,
                input_slave_idle_percent, _results.global_clock_counter,
                input_mbps)
         << std::endl;

  result << string_format(
                "output\t%-17lu  %-9lu%3.0f%%  %-9lu%3.0f%%  %-9lu%3.0f%%  "
                "%-12lu  %-4.2f",
                _results.output_data_byte_count,
                _results.output_transfer_cycle_count,
                output_transfer_cycle_percent,
                _results.output_master_idle_count, output_master_idle_percent,
                _results.output_slave_idle_count, output_slave_idle_percent,
                _results.global_clock_counter, output_mbps)
         << std::endl;

  return result.str();
}

template <typename... Args>
std::string ProfilingPipelineRunner::string_format(const std::string &format,
                                                   Args... args) {
  // From: https://stackoverflow.com/a/26221725

  size_t size = snprintf(nullptr, 0, format.c_str(), args...) +
                1;  // Extra space for '\0'
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(),
                     buf.get() + size - 1);  // We don't want the '\0' inside
}

uint64_t MockPipelineRunner::run(bool last) {
  (void)last;
  return 0;
}

}  // namespace metal
