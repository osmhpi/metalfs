#include "pipeline.h"

#include <pthread.h>

#include "../metal/metal.h"

pthread_mutex_t snap_mutex = PTHREAD_MUTEX_INITIALIZER;

int mtl_pipeline_initialize() {
    return MTL_SUCCESS;
}

int mtl_pipeline_deinitialize() {
    return MTL_SUCCESS;
}

void mtl_configure_afu(mtl_afu_specification *afu_spec) {
    pthread_mutex_lock(&snap_mutex);
    afu_spec->apply_config();
    pthread_mutex_unlock(&snap_mutex);
}

void mtl_configure_pipeline(mtl_afu_execution_plan execution_plan) {
    pthread_mutex_lock(&snap_mutex);
    pthread_mutex_unlock(&snap_mutex);
}

void mtl_run_pipeline() {
    pthread_mutex_lock(&snap_mutex);
    pthread_mutex_unlock(&snap_mutex);
}
