extern "C" {
#include <unistd.h>
#include <snap_hls_if.h>
}

#include <metal_fpga/hw/hls/include/action_metalfpga.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include "pipeline_runner.hpp"
#include "snap_action.hpp"

namespace metal {

void SnapPipelineRunner::run(bool run_finalize) {

    SnapAction action = SnapAction(METALFPGA_ACTION_TYPE, _card);

    if (!_initialized) {
        initialize(action);
        _initialized = true;
    }

    _pipeline->run(action);

    if (run_finalize) {
        finalize(action);
    }
}

void ProfilingPipelineRunner::initialize(SnapAction &action) {

    if (_op != nullptr) {
        auto operatorPosition = std::find(_pipeline->operators().begin(), _pipeline->operators().end(), _op);

        if (operatorPosition == _pipeline->operators().cend())
            throw std::runtime_error("Operator to be profiled must be part of pipeline");

        auto *job_struct = (uint64_t*)snap_malloc(sizeof(uint64_t) * 2);

        if (operatorPosition == _pipeline->operators().begin()) {
            // If the operator is the data source, we profile its output stream twice
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

        std::cout << "Input stream " << job_struct[0] << std::endl;
        std::cout << "Output stream " << job_struct[1] << std::endl;

        try {
            action.execute_job(MTL_JOB_CONFIGURE_PERFMON, reinterpret_cast<char *>(job_struct));
        } catch (std::exception &ex) {
            free(job_struct);
            throw ex;
        }

        free(job_struct);
    }
}

void ProfilingPipelineRunner::finalize(SnapAction &action) {
    if (_op != nullptr) {
        auto *job_struct = (uint64_t*)snap_malloc(sizeof(uint64_t) * 6);

        try {
            action.execute_job(MTL_JOB_READ_PERFMON_COUNTERS, reinterpret_cast<char *>(job_struct));
        } catch (std::exception &ex) {
            free(job_struct);
            throw ex;
        }

        _global_clock_counter = htobe64(job_struct[0]);

        memcpy(_performance_counters.data(), &job_struct[1], 10 * sizeof(uint32_t));

        free(job_struct);
    }
}

void ProfilingPipelineRunner::selectOperatorForProfiling(std::shared_ptr<AbstractOperator> op) {
    _op = std::move(op);
    requireReinitiailzation();
}

void ProfilingPipelineRunner::printProfilingResults() {
    const double freq = 250;
    const double onehundred = 100;

    uint32_t input_data_byte_count = be32toh(_performance_counters[2]);
    uint32_t input_transfer_cycle_count = be32toh(_performance_counters[0]);
    double input_transfer_cycle_percent = input_transfer_cycle_count * onehundred / _global_clock_counter;
    uint32_t input_slave_idle_count = be32toh(_performance_counters[3]);
    double input_slave_idle_percent = input_slave_idle_count * onehundred / _global_clock_counter;
    uint32_t input_master_idle_count = be32toh(_performance_counters[4]);
    double input_master_idle_percent = input_master_idle_count * onehundred / _global_clock_counter;
    double input_mbps = (input_data_byte_count * freq) / (double)_global_clock_counter;

    uint32_t output_data_byte_count = be32toh(_performance_counters[7]);
    uint32_t output_transfer_cycle_count = be32toh(_performance_counters[5]);
    double output_transfer_cycle_percent = output_transfer_cycle_count * onehundred / _global_clock_counter;
    uint32_t output_slave_idle_count = be32toh(_performance_counters[8]);
    double output_slave_idle_percent = output_slave_idle_count * onehundred / _global_clock_counter;
    uint32_t output_master_idle_count = be32toh(_performance_counters[9]);
    double output_master_idle_percent = output_master_idle_count * onehundred / _global_clock_counter;
    double output_mbps = (output_data_byte_count * freq) / (double)_global_clock_counter;

    printf("STREAM\tBYTES TRANSFERRED  ACTIVE CYCLES  DATA WAIT      CONSUMER WAIT  TOTAL CYCLES  MB/s\n");

    printf("input\t%-17u  %-9u%3.0f%%  %-9u%3.0f%%  %-9u%3.0f%%  %-12lu  %-4.2f\n",
            input_data_byte_count, input_transfer_cycle_count, input_transfer_cycle_percent, input_master_idle_count,
            input_master_idle_percent, input_slave_idle_count, input_slave_idle_percent, _global_clock_counter, input_mbps);

    printf("output\t%-17u  %-9u%3.0f%%  %-9u%3.0f%%  %-9u%3.0f%%  %-12lu  %-4.2f\n",
            output_data_byte_count, output_transfer_cycle_count, output_transfer_cycle_percent, output_master_idle_count,
            output_master_idle_percent, output_slave_idle_count, output_slave_idle_percent, _global_clock_counter, output_mbps);
}

void MockPipelineRunner::run(bool finalize) {
    (void)finalize;
}

} // namespace metal
