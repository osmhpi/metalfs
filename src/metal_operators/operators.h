#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct operator_id { uint8_t enable_id; uint8_t stream_id; } operator_id;
struct snap_action;

typedef struct mtl_operator_invocation_args {
    const char* cwd;
    const char* metal_mountpoint;
    int argc;
    char **argv;
} mtl_operator_invocation_args;

typedef const void* (*mtl_operator_handle_opts_f)(mtl_operator_invocation_args *args, uint64_t *length, bool *valid);
typedef int (*mtl_operator_apply_configuration_f)(struct snap_action *action);
typedef int (*mtl_operator_finalize_f)(struct snap_action *action);
typedef const char* (*mtl_operator_get_filename_f)();

typedef struct mtl_operator_specification {
    operator_id id;
    const char *name;
    bool is_data_source;

    mtl_operator_handle_opts_f handle_opts;
    mtl_operator_apply_configuration_f apply_config;
    mtl_operator_finalize_f finalize;
    mtl_operator_get_filename_f get_filename;
} mtl_operator_specification;

typedef struct mtl_operator_execution_plan {
    const operator_id* operators;
    uint64_t length;
} mtl_operator_execution_plan;
