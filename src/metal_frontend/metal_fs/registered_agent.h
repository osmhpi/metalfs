#pragma once

#include <stdint.h>
#include <pthread.h>

#include "../../metal_operators/operators.h"

#include "list/list.h"

extern pthread_mutex_t registered_agent_mutex;

typedef struct registered_agent {
    int pid;
    operator_id op_type;
    mtl_operator_specification *op_specification;
    int socket;

    char cwd[FILENAME_MAX];
    char metal_mountpoint[FILENAME_MAX];
    int argc;
    char *argv_buffer;
    char **argv;

    // bool start_was_signaled;

    struct registered_agent* input_agent;
    int input_agent_pid;
    int input_file;
    char* input_buffer;

    char *internal_input_file;
    char *internal_output_file;

    // int internal_input_buffer_handle;
    // char* internal_input_size;
    // bool internal_input_eof;
    // pthread_mutex_t* internal_input_mutex;
    // pthread_cond_t* internal_input_condition;

    struct registered_agent* output_agent;
    int output_agent_pid;
    int output_file;
    char* output_buffer;

    LIST_ENTRY Link;
} registered_agent_t;

// void signal_new_execution_plan(mtl_operator_execution_plan plan);
// void* agent_thread(void* args);
