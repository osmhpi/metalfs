#include "pipeline.h"

#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <errno.h>
#include <pthread.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

#include "../metal_fpga/include/action_metalfpga.h"
#include "../metal_storage/storage.h"

#include "../metal/metal.h"

pthread_mutex_t snap_mutex = PTHREAD_MUTEX_INITIALIZER;

struct snap_action *_action = NULL;
struct snap_card *_card = NULL;

int mtl_pipeline_initialize() {
    char device[128];
    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", 0);

    if (_card == NULL) {
        _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
        if (_card == NULL) {
            fprintf(stderr, "err: failed to open card %u: %s\n", 0, strerror(errno));
            return MTL_ERROR_INVALID_ARGUMENT;
        }
    }

    if (_action == NULL) {
        snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
        _action = snap_attach_action(_card, METALFPGA_ACTION_TYPE, action_irq, 60);
        if (_action == NULL) {
            fprintf(stderr, "err: failed to attach action %u: %s\n", 0, strerror(errno));
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

void mtl_configure_afu(mtl_operator_specification *afu_spec) {
    pthread_mutex_lock(&snap_mutex);

    afu_spec->apply_config(_action);

    pthread_mutex_unlock(&snap_mutex);
}

void mtl_configure_pipeline(mtl_operator_execution_plan execution_plan) {
    uint64_t enable_mask = 0;
    for (uint64_t i = 0; i < execution_plan.length; ++i)
        enable_mask |= (1 << execution_plan.afus[i].enable_id);

    uint64_t *job_struct_enable = (uint64_t*)snap_malloc(sizeof(uint32_t) * 10);
    *job_struct_enable = htobe64(enable_mask);

    uint32_t *job_struct = (uint32_t*)(job_struct_enable + 1);
    const uint32_t disable = 0x80000000;
    for (int i = 0; i < 8; ++i)
        job_struct[i] = htobe32(disable);

    uint8_t previous_afu_stream = execution_plan.length ? execution_plan.afus[0].stream_id : 0;
    for (uint64_t i = 1; i < execution_plan.length; ++i) {
        // From the perspective of the Stream Switch:
        // Which Master port (output) should be
        // sourced from which Slave port (input)
        job_struct[execution_plan.afus[i].stream_id] = htobe32(previous_afu_stream);
        previous_afu_stream = execution_plan.afus[i].stream_id;
    }

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_CONFIGURE_STREAMS;
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
    mjob.job_type = MF_JOB_RUN_AFUS;
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

void mtl_pipeline_set_file_read_extent_list(const mtl_file_extent *extents, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(
        sizeof(uint64_t) * (
            8 // words for the prefix
            + (2 * length) // two words for each extent
        )
    );

    job_words[0] = 0;  // slot number
    job_words[1] = true;  // map (vs unmap)
    job_words[2] = length;  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        job_words[8 + 2*i + 0] = extents[i].offset;
        job_words[8 + 2*i + 1] = extents[i].length;
    }

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_MAP;
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

    job_words[0] = 1;  // slot number
    job_words[1] = true;  // map (vs unmap)
    job_words[2] = length;  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        job_words[8 + 2*i + 0] = extents[i].offset;
        job_words[8 + 2*i + 1] = extents[i].length;
    }

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_MAP;
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
