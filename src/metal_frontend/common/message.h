#pragma once

#include <stdint.h>
#include <netinet/in.h>

#include "../../metal_operators/operators.h"

enum class message_type : int {
    AGENT_HELLO,
    AGENT_PUSH_BUFFER,

    SERVER_ACCEPT_AGENT,
    SERVER_INITIALIZE_OUTPUT_BUFFER,
    SERVER_PROCESSED_BUFFER,
    SERVER_GOODBYE,

    ERROR
};

//
//typedef struct agent_hello_data {
//    uint64_t pid;
//    char cwd[FILENAME_MAX];
//    char metal_mountpoint[FILENAME_MAX]; // There should actually be no need to tell the fs server about this
//    operator_id op_type;
//    uint64_t input_agent_pid;
//    uint64_t output_agent_pid;
//    uint64_t argc;
//    uint64_t argv_len;
//    char input_buffer_filename[FILENAME_MAX];
//    char internal_input_filename[FILENAME_MAX];
//    char internal_output_filename[FILENAME_MAX];
//} agent_hello_data_t;
//
//typedef struct agent_push_buffer_data {
//    uint64_t size;
//    bool eof;
//} agent_push_buffer_data_t;
//
//typedef struct server_initialize_output_buffer_data {
//    char output_buffer_filename[FILENAME_MAX];
//} server_initialize_output_buffer_data_t;
//
//typedef struct server_accept_agent_data {
//    uint64_t response_length;
//    bool valid;
//} server_accept_agent_data_t;
//
//typedef struct server_processed_buffer_data {
//    uint64_t size;
//    bool eof;
//    uint64_t message_length;
//} server_processed_buffer_data_t;
