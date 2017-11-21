#pragma once

#include <stdint.h>

typedef enum message_type {
    AGENT_HELLO,
    AGENT_PUSH_BUFFER,

    SERVER_ACCEPT_AGENT,
    SERVER_PROCESSED_BUFFER,
    SERVER_GOODBYE,

    ERROR
} message_type_t;


typedef struct agent_hello_data {
    uint64_t pid;
    int afu_type;
    uint64_t input_agent_pid;
    char input_buffer_filename[232];
} agent_hello_data_t;

typedef struct agent_push_buffer_data {
    uint64_t size;
    bool eof;
} agent_push_buffer_data_t;

typedef struct server_processed_buffer_data {
    uint64_t size;
    bool eof;
    char output_buffer_filename[244];
} server_processed_buffer_data_t;

typedef struct message {
    message_type_t type;
    union {
        char message[252];

        agent_hello_data_t agent_hello;
        agent_push_buffer_data_t agent_push_buffer;

        server_processed_buffer_data_t server_processed_buffer;
    } data;
} message_t;


