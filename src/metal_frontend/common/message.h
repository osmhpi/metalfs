#pragma once

#include <stdint.h>

#include "../../metal_operators/operators.h"

typedef enum message_type {
    AGENT_HELLO,
    AGENT_PUSH_BUFFER,

    SERVER_ACCEPT_AGENT,
    SERVER_INITIALIZE_OUTPUT_BUFFER,
    SERVER_PROCESSED_BUFFER,
    SERVER_GOODBYE,

    ERROR
} message_type_t;

typedef struct agent_hello_data {
    uint64_t pid;
    operator_id afu_type;
    uint64_t input_agent_pid;
    uint64_t output_agent_pid;
    uint64_t argc;
    uint64_t argv_len;
    char input_buffer_filename[FILENAME_MAX];
    char internal_input_filename[FILENAME_MAX];
    char internal_output_filename[FILENAME_MAX];
} agent_hello_data_t;

typedef struct agent_push_buffer_data {
    uint64_t size;
    bool eof;
} agent_push_buffer_data_t;

typedef struct server_initialize_output_buffer_data {
    char output_buffer_filename[FILENAME_MAX];
} server_initialize_output_buffer_data_t;

typedef struct server_accept_agent_data {
    uint64_t response_length;
    bool valid;
} server_accept_agent_data_t;

typedef struct server_processed_buffer_data {
    uint64_t size;
    bool eof;
} server_processed_buffer_data_t;
