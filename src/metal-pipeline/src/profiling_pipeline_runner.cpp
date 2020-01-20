#include <metal-pipeline/profiling_pipeline_runner.hpp>

extern "C" {
#include <unistd.h>
}

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

#include <snap_action_metal.h>
#include <metal-pipeline/common.hpp>
#include <metal-pipeline/data_sink_context.hpp>
#include <metal-pipeline/data_source_context.hpp>
#include <metal-pipeline/operator_specification.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

void ProfilingPipelineRunner::preRun(SnapAction &action,
                                     DataSourceContext &dataSource,
                                     DataSinkContext &dataSink,
                                     bool initialize) {
  if (initialize) {
    _profileStreamIds = std::nullopt;

    auto operatorPosition = _pipeline->operators().cend();
    for (auto it = _pipeline->operators().cbegin();
         it != _pipeline->operators().cend(); ++it) {
      if (it->profiling_enabled()) {
        if (operatorPosition == _pipeline->operators().cend()) {
          operatorPosition = it;
        } else {
          throw std::runtime_error(
              "Only one operator can be selected for profiling.");
        }
      }
    }

    {
      std::vector<bool> operatorsToProfile{
          operatorPosition != _pipeline->operators().cend(),
          dataSource.profilingEnabled(), dataSink.profilingEnabled()};
      if (std::count(operatorsToProfile.begin(), operatorsToProfile.end(),
                     true) > 1) {
        throw std::runtime_error(
            "Only one operator can be selected for profiling.");
      }
    }

    if (operatorPosition != _pipeline->operators().cend()) {
      auto inputStreamId = operatorPosition->userOperator().spec().streamID();

      if (++operatorPosition == _pipeline->operators().cend()) {
        _profileStreamIds = std::make_pair(inputStreamId, IOStreamID);
      } else {
        _profileStreamIds = std::make_pair(
            inputStreamId, operatorPosition->userOperator().spec().streamID());
      }
    }

    if (dataSource.profilingEnabled()) {
      _profileStreamIds = std::make_pair(IOStreamID, IOStreamID);
    }

    if (dataSink.profilingEnabled()) {
      if (_pipeline->operators().empty()) {
        _profileStreamIds = std::make_pair(IOStreamID, IOStreamID);
      } else {
        _profileStreamIds = std::make_pair(
            _pipeline->operators().back().userOperator().spec().streamID(),
            _pipeline->operators().back().userOperator().spec().streamID());
      }
    }

    if (_profileStreamIds) {
      auto *job_struct = reinterpret_cast<uint64_t *>(
          action.allocateMemory(sizeof(uint64_t) * 2));

      job_struct[0] = htobe64(_profileStreamIds->first);
      job_struct[1] = htobe64(_profileStreamIds->second);

      try {
        action.execute_job(fpga::JobType::ConfigurePerfmon,
                           reinterpret_cast<char *>(job_struct));
      } catch (std::exception &ex) {
        free(job_struct);
        throw ex;
      }

      free(job_struct);
    }
  }

  if (_profileStreamIds) {
    action.execute_job(fpga::JobType::ResetPerfmon);
  }

  SnapPipelineRunner::preRun(action, dataSource, dataSink, initialize);
}  // namespace metal

void ProfilingPipelineRunner::postRun(SnapAction &action,
                                      DataSourceContext &dataSource,
                                      DataSinkContext &dataSink,
                                      bool finalize) {
  if (_profileStreamIds) {
    auto *results64 = reinterpret_cast<uint64_t *>(
        action.allocateMemory(sizeof(uint64_t) * 6));
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

    if (finalize) {
      if (dataSource.profilingEnabled()) {
        dataSource.setProfilingResults(formatProfilingResults());
      } else if (dataSink.profilingEnabled()) {
        dataSink.setProfilingResults(formatProfilingResults());
      } else {
        auto op = std::find_if(_pipeline->operators().begin(),
                               _pipeline->operators().end(),
                               [&](OperatorContext &op) {
                                 return op.userOperator().spec().streamID() ==
                                        _profileStreamIds->first;
                               });
        if (op != _pipeline->operators().cend()) {
          op->setProfilingResults(formatProfilingResults());
        }
      }
    }
  }

  SnapPipelineRunner::postRun(action, dataSource, dataSink, finalize);
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

}  // namespace metal
