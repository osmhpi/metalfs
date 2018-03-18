#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t afu_id;
struct snap_action;

typedef const void* (*mtl_afu_handle_opts_f)(int argc, char *argv[], uint64_t *length, bool *valid);
typedef int (*mtl_afu_apply_configuration_f)(struct snap_action *action);

typedef struct mtl_afu_specification {
    afu_id id;
    const char *name;

    mtl_afu_handle_opts_f handle_opts;
    mtl_afu_apply_configuration_f apply_config;
} mtl_afu_specification;

typedef struct mtl_afu_execution_plan {
    const afu_id* afus;
    uint64_t length;
} mtl_afu_execution_plan;
