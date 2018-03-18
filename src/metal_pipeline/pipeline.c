#include "pipeline.h"

#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <errno.h>
#include <pthread.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

#include "action_metalfpga.h"

#include "../metal/metal.h"

pthread_mutex_t snap_mutex = PTHREAD_MUTEX_INITIALIZER;

struct snap_action *_action = NULL;
struct snap_card *_card = NULL;

int mtl_pipeline_initialize() {
    char device[128];
    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", 0);

    _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
    if (_card == NULL) {
        fprintf(stderr, "err: failed to open card %u: %s\n", 0, strerror(errno));
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
    _action = snap_attach_action(_card, METALFPGA_ACTION_TYPE, action_irq, 60);
    if (_action == NULL) {
        fprintf(stderr, "err: failed to attach action %u: %s\n", 0, strerror(errno));
        snap_card_free(_card);
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    return MTL_SUCCESS;
}

int mtl_pipeline_deinitialize() {
    snap_detach_action(_action);
    snap_card_free(_card);
    return MTL_SUCCESS;
}

void mtl_configure_afu(mtl_afu_specification *afu_spec) {
    pthread_mutex_lock(&snap_mutex);

    afu_spec->apply_config(_action);

    pthread_mutex_unlock(&snap_mutex);
}

void mtl_configure_pipeline(mtl_afu_execution_plan execution_plan) {
    pthread_mutex_lock(&snap_mutex);

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
        job_struct[previous_afu_stream] = execution_plan.afus[i].stream_id;
        previous_afu_stream = execution_plan.afus[i].stream_id;
    }

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_CONFIGURE_STREAMS;
    mjob.job_address = (uint64_t)job_struct_enable;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    const unsigned long timeout = 600;
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

void mtl_run_pipeline() {
    pthread_mutex_lock(&snap_mutex);
    pthread_mutex_unlock(&snap_mutex);
}
