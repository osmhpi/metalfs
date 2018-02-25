#pragma once

#include <stdint.h>
#include <pthread.h>
#include "list/list.h"

extern pthread_mutex_t registered_agent_mutex;

typedef struct registered_agent {
    int pid;
    int afu_type;
    int socket;

    int argc;
    uint64_t argv_len;

    struct registered_agent* input_agent;
    int input_agent_pid;
    int input_file;
    char* input_buffer;

    int internal_input_buffer_handle;
    char* internal_input_size;
    bool internal_input_eof;
    pthread_mutex_t* internal_input_mutex;
    pthread_cond_t* internal_input_condition;

    struct registered_agent* output_agent;
    int output_file;
    char* output_buffer;

    LIST_ENTRY Link;
} registered_agent_t;

void* agent_thread(void* args);
