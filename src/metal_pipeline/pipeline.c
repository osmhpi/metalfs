#include "pipeline.h"

#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <errno.h>
#include <pthread.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

#include "../metal_fpga/hw/hls/include/action_metalfpga.h"
#include "../metal_storage/storage.h"

#include "../metal/metal.h"

pthread_mutex_t snap_mutex = PTHREAD_MUTEX_INITIALIZER;

struct snap_action *_action = NULL;
struct snap_card *_card = NULL;

int mtl_pipeline_initialize() {
    int card_no = 0;
    char device[128];
    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", card_no);

    if (_card == NULL) {
        _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
        if (_card == NULL) {
            fprintf(stderr, "err: failed to open card %u: %s\n", card_no, strerror(errno));
            return MTL_ERROR_INVALID_ARGUMENT;
        }
    }

    if (_action == NULL) {
        snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
        _action = snap_attach_action(_card, METALFPGA_ACTION_TYPE, action_irq, 30);
        if (_action == NULL) {
            fprintf(stderr, "err: failed to attach action %u: %s\n", card_no, strerror(errno));
            snap_card_free(_card);
            return MTL_ERROR_INVALID_ARGUMENT;
        }
    }

    return MTL_SUCCESS;
}

int mtl_pipeline_deinitialize() {
    snap_detach_action(_action);
    snap_card_free(_card);

    _action = NULL;
    _card = NULL;

    return MTL_SUCCESS;
}

void mtl_configure_operator(mtl_operator_specification *op_spec) {
    pthread_mutex_lock(&snap_mutex);

    op_spec->apply_config(_action);

    pthread_mutex_unlock(&snap_mutex);
}

void mtl_finalize_operator(mtl_operator_specification *op_spec) {
    if (op_spec->finalize) {
        pthread_mutex_lock(&snap_mutex);

        op_spec->finalize(_action);

        pthread_mutex_unlock(&snap_mutex);
    }
}

void mtl_configure_pipeline(mtl_operator_execution_plan execution_plan) {
    uint64_t enable_mask = 0;
    for (uint64_t i = 0; i < execution_plan.length; ++i)
        enable_mask |= (1 << execution_plan.operators[i].enable_id);

    uint64_t *job_struct_enable = (uint64_t*)snap_malloc(sizeof(uint32_t) * 10);
    *job_struct_enable = htobe64(enable_mask);

    uint32_t *job_struct = (uint32_t*)(job_struct_enable + 1);
    const uint32_t disable = 0x80000000;
    for (int i = 0; i < 8; ++i)
        job_struct[i] = htobe32(disable);

    uint8_t previous_op_stream = execution_plan.length ? execution_plan.operators[0].stream_id : 0;
    for (uint64_t i = 1; i < execution_plan.length; ++i) {
        // From the perspective of the Stream Switch:
        // Which Master port (output) should be
        // sourced from which Slave port (input)
        job_struct[execution_plan.operators[i].stream_id] = htobe32(previous_op_stream);
        previous_op_stream = execution_plan.operators[i].stream_id;
    }

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_CONFIGURE_STREAMS;
    mjob.job_address = (uint64_t)job_struct_enable;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    free(job_struct_enable);

    if (rc != 0)
        // Some error occurred
        return;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return;

    return;
}

void mtl_run_pipeline() {
    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_RUN_OPERATORS;
    mjob.job_address = 0;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    if (rc != 0)
        // Some error occurred
        return;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return;

    return;
}


void mtl_reset_perfmon() {
    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_RESET_PERFMON;
    mjob.job_address = 0;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    if (rc != 0)
        // Some error occurred
        return;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return;

    return;
}

void mtl_configure_perfmon(uint64_t stream_id0, uint64_t stream_id1) {
    uint64_t *job_struct = (uint64_t*)snap_malloc(sizeof(uint64_t));
    job_struct[0] = htobe64(stream_id0);
    job_struct[1] = htobe64(stream_id1);

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_CONFIGURE_PERFMON;
    mjob.job_address = (uint64_t)job_struct;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    free(job_struct);

    if (rc != 0)
        // Some error occurred
        return;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return;

    return;
}

uint64_t mtl_read_perfmon(char * buffer, uint64_t buffer_size) {
    uint64_t *job_struct = (uint64_t*)snap_malloc(sizeof(uint64_t) * 6);
    uint32_t *counters = (uint32_t*)(job_struct + 1);

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_READ_PERFMON_COUNTERS;
    mjob.job_address = (uint64_t)job_struct;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    if (rc != 0) {
        // Some error occurred
        free(job_struct);
        return 0;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        // Some error occurred
        free(job_struct);
        return 0;
    }

    char *current_offset = buffer;
    const double freq = 250;
    const double onehundred = 100;

    uint64_t global_clock_count = be64toh(job_struct[0]);

    uint32_t input_data_byte_count = be32toh(counters[2]);
    uint32_t input_transfer_cycle_count = be32toh(counters[0]);
    double input_transfer_cycle_percent = input_transfer_cycle_count * onehundred / global_clock_count;
    uint32_t input_slave_idle_count = be32toh(counters[3]);
    double input_slave_idle_percent = input_slave_idle_count * onehundred / global_clock_count;
    uint32_t input_master_idle_count = be32toh(counters[4]);
    double input_master_idle_percent = input_master_idle_count * onehundred / global_clock_count;
    double input_mbps = (input_data_byte_count * freq) / (double)global_clock_count;

    uint32_t output_data_byte_count = be32toh(counters[7]);
    uint32_t output_transfer_cycle_count = be32toh(counters[5]);
    double output_transfer_cycle_percent = output_transfer_cycle_count * onehundred / global_clock_count;
    uint32_t output_slave_idle_count = be32toh(counters[8]);
    double output_slave_idle_percent = output_slave_idle_count * onehundred / global_clock_count;
    uint32_t output_master_idle_count = be32toh(counters[9]);
    double output_master_idle_percent = output_master_idle_count * onehundred / global_clock_count;
    double output_mbps = (output_data_byte_count * freq) / (double)global_clock_count;

    current_offset += snprintf(current_offset, buffer + buffer_size - current_offset, "STREAM\tBYTES TRANSFERRED  ACTIVE CYCLES  DATA WAIT      CONSUMER WAIT  TOTAL CYCLES  MB/s\n");

    current_offset += snprintf(current_offset, buffer + buffer_size - current_offset, "input\t%-17u  %-9u%3.0f%%  %-9u%3.0f%%  %-9u%3.0f%%  %-12lu  %-4.2f\n", input_data_byte_count, input_transfer_cycle_count, input_transfer_cycle_percent, input_master_idle_count, input_master_idle_percent, input_slave_idle_count, input_slave_idle_percent, global_clock_count, input_mbps);
    current_offset += snprintf(current_offset, buffer + buffer_size - current_offset, "output\t%-17u  %-9u%3.0f%%  %-9u%3.0f%%  %-9u%3.0f%%  %-12lu  %-4.2f\n", output_data_byte_count, output_transfer_cycle_count, output_transfer_cycle_percent, output_master_idle_count, output_master_idle_percent, output_slave_idle_count, output_slave_idle_percent, global_clock_count, output_mbps);

    return current_offset - buffer;
}

void mtl_pipeline_set_file_read_extent_list(const mtl_file_extent *extents, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(
        sizeof(uint64_t) * (
            8 // words for the prefix
            + (2 * length) // two words for each extent
        )
    );

    // printf("Mapping %lu extents for reading\n", length);
    job_words[0] = htobe64(0);  // slot number
    job_words[1] = htobe64(true);  // map (vs unmap)
    job_words[2] = htobe64(length);  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        // printf("   Extent %lu has length %lu and starts at %lu\n", i, extents[i].length, extents[i].offset);
        job_words[8 + 2*i + 0] = htobe64(extents[i].offset);
        job_words[8 + 2*i + 1] = htobe64(extents[i].length);
    }

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_MAP;
    mjob.job_address = (uint64_t)job_words;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    free(job_words);

    if (rc != 0) {
        return;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        return;
    }
}

void mtl_pipeline_set_file_write_extent_list(const mtl_file_extent *extents, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(
        sizeof(uint64_t) * (
            8 // words for the prefix
            + (2 * length) // two words for each extent
        )
    );

    // printf("Mapping %lu extents for writing\n", length);
    job_words[0] = htobe64(1);  // slot number
    job_words[1] = htobe64(true);  // map (vs unmap)
    job_words[2] = htobe64(length);  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        // printf("   Extent %lu has length %lu and starts at %lu\n", i, extents[i].length, extents[i].offset);
        job_words[8 + 2*i + 0] = htobe64(extents[i].offset);
        job_words[8 + 2*i + 1] = htobe64(extents[i].length);
    }

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_MAP;
    mjob.job_address = (uint64_t)job_words;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    free(job_words);

    if (rc != 0) {
        return;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        return;
    }
}
