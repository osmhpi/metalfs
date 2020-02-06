#include <metal-pipeline/profiling_pipeline_runner.hpp>

extern "C" {
#include <unistd.h>
}

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

#include <spdlog/spdlog.h>

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
      if (it->profilingEnabled()) {
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
      if (_pipeline->operators().empty()) {
        _profileStreamIds = std::make_pair(IOStreamID, IOStreamID);
      } else {
        _profileStreamIds = std::make_pair(
            _pipeline->operators().front().userOperator().spec().streamID(),
            _pipeline->operators().front().userOperator().spec().streamID());
      }
    }

    if (dataSink.profilingEnabled()) {
      _profileStreamIds = std::make_pair(IOStreamID, IOStreamID);
    }

    if (_profileStreamIds) {
      spdlog::debug("Selecting streams {} and {} for profiling.", _profileStreamIds->first, _profileStreamIds->second);
      auto *job_struct = reinterpret_cast<uint64_t *>(
          action.allocateMemory(sizeof(uint64_t) * 2));

      job_struct[0] = htobe64(_profileStreamIds->first);
      job_struct[1] = htobe64(_profileStreamIds->second);

      try {
        action.executeJob(fpga::JobType::ConfigurePerfmon,
                          reinterpret_cast<char *>(job_struct));
      } catch (std::exception &ex) {
        free(job_struct);
        throw ex;
      }

      free(job_struct);
    }
  }

  if (_profileStreamIds) {
    action.executeJob(fpga::JobType::ResetPerfmon);
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
      action.executeJob(fpga::JobType::ReadPerfmonCounters,
                        reinterpret_cast<char *>(results64));
    } catch (std::exception &ex) {
      free(results64);
      throw ex;
    }

    _results.globalClockCounter += be64toh(results64[0]);

    _results.inputTransferCycleCount += be32toh(results32[0]);
    _results.inputDataByteCount += be32toh(results32[2]);
    _results.inputSlaveIdleCount += be32toh(results32[3]);
    _results.inputMasterIdleCount += be32toh(results32[4]);

    _results.outputTransferCycleCount += be32toh(results32[5]);
    _results.outputDataByteCount += be32toh(results32[7]);
    _results.outputSlaveIdleCount += be32toh(results32[8]);
    _results.outputMasterIdleCount += be32toh(results32[9]);

    free(results64);

    if (finalize) {
      if (dataSource.profilingEnabled()) {
        dataSource.setProfilingResults(formatProfilingResults(true, false));
      } else if (dataSink.profilingEnabled()) {
        dataSink.setProfilingResults(formatProfilingResults(false, true));
      } else {
        auto op = std::find_if(_pipeline->operators().begin(),
                               _pipeline->operators().end(),
                               [&](OperatorContext &op) {
                                 return op.userOperator().spec().streamID() ==
                                        _profileStreamIds->first;
                               });
        if (op != _pipeline->operators().cend()) {
          op->setProfilingResults(formatProfilingResults(false, false));
        }
      }
    }
  }

  SnapPipelineRunner::postRun(action, dataSource, dataSink, finalize);
}

std::string ProfilingPipelineRunner::formatProfilingResults(bool dataSource, bool dataSink) {
  const double freq = 250;
  const double onehundred = 100;

  double input_transfer_cycle_percent = _results.inputTransferCycleCount *
                                        onehundred /
                                        _results.globalClockCounter;
  double input_slave_idle_percent =
      _results.inputSlaveIdleCount * onehundred / _results.globalClockCounter;
  double input_master_idle_percent =
      _results.inputMasterIdleCount * onehundred / _results.globalClockCounter;
  double input_mbps = (_results.inputDataByteCount * freq) /
                      (double)_results.globalClockCounter;

  double output_transfer_cycle_percent = _results.outputTransferCycleCount *
                                         onehundred /
                                         _results.globalClockCounter;
  double output_slave_idle_percent =
      _results.outputSlaveIdleCount * onehundred / _results.globalClockCounter;
  double output_master_idle_percent =
      _results.outputMasterIdleCount * onehundred / _results.globalClockCounter;
  double output_mbps = (_results.outputDataByteCount * freq) /
                       (double)_results.globalClockCounter;

  std::stringstream result;

  result << "STREAM\tBYTES TRANSFERRED  ACTIVE CYCLES  DATA WAIT      CONSUMER "
            "WAIT  TOTAL CYCLES  MiB/s"
         << std::endl;

  if (!dataSource) {
    result << string_format(
                  "input\t%-17lu  %-9lu%3.0f%%  %-9lu%3.0f%%  %-9lu%3.0f%%  "
                  "%-12lu  %-4.2f",
                  _results.inputDataByteCount, _results.inputTransferCycleCount,
                  input_transfer_cycle_percent, _results.inputMasterIdleCount,
                  input_master_idle_percent, _results.inputSlaveIdleCount,
                  input_slave_idle_percent, _results.globalClockCounter,
                  input_mbps)
           << std::endl;
   }

   if (!dataSink) {
  result << string_format(
                "output\t%-17lu  %-9lu%3.0f%%  %-9lu%3.0f%%  %-9lu%3.0f%%  "
                "%-12lu  %-4.2f",
                _results.outputDataByteCount, _results.outputTransferCycleCount,
                output_transfer_cycle_percent, _results.outputMasterIdleCount,
                output_master_idle_percent, _results.outputSlaveIdleCount,
                output_slave_idle_percent, _results.globalClockCounter,
                output_mbps)
         << std::endl;
   }

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
