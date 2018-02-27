#include "pipeline.h"

#include <pthread.h>

pthread_mutex_t pipeline_mutex = PTHREAD_MUTEX_INITIALIZER;

void mtl_configure_afu(mtl_afu_specification *afu_spec) {
    pthread_mutex_lock(&pipeline_mutex);
    pthread_mutex_unlock(&pipeline_mutex);
}

void mtl_configure_pipeline(mtl_afu_execution_plan execution_plan) {
    pthread_mutex_lock(&pipeline_mutex);
    pthread_mutex_unlock(&pipeline_mutex);
}

void mtl_run_pipeline() {
    pthread_mutex_lock(&pipeline_mutex);
    pthread_mutex_unlock(&pipeline_mutex);
}
