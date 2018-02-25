#include "agent_worker.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pthread.h>

#include "../common/known_afus.h"
#include "../common/message.h"
#include "../common/buffer.h"

pthread_mutex_t registered_agent_mutex = PTHREAD_MUTEX_INITIALIZER;

void* agent_thread(void* args) {
    registered_agent_t *agent = args;

    bool valid = false;

    // Look up the AFU specification that should be used
    mtl_afu_specification *afu_spec = NULL;
    for (uint64_t i = 0; i < sizeof(known_afus) / sizeof(known_afus[0]); ++i) {
        if (known_afus[i]->id == agent->afu_type) {
            afu_spec = known_afus[i];
            break;
        }
    }

    // See if the options provided allow to execute the AFU
    const char* response = NULL;
    uint64_t response_len = 0;
    if (afu_spec && agent->argc) {
        char argv_buffer[agent->argv_len];
        recv(agent->socket, &argv_buffer, agent->argv_len, 0);
        char *argv[agent->argc];
        uint64_t current_arg = 0;
        void *argv_cursor = argv_buffer;
        while (argv_cursor < argv_buffer + agent->argv_len) {
            argv[current_arg++] = argv_cursor;
            argv_cursor += strlen(argv_cursor) + 1;
        }

        response = afu_spec->handle_opts(agent->argc, argv, &response_len, &valid);
    }

    message_type_t message_type = SERVER_ACCEPT_AGENT;
    send(agent->socket, &message_type, sizeof(message_type), 0);
    server_accept_agent_data_t accept_data = { response_len, valid };
    send(agent->socket, &accept_data, sizeof(accept_data), 0);
    if (response_len) {
        send(agent->socket, response, response_len, 0);
    }

    while (valid) {
        message_type_t incoming_message_type;
        size_t received = recv(agent->socket, &incoming_message_type, sizeof(incoming_message_type), 0);
        if (received == 0)
            break;

        if (incoming_message_type != AGENT_PUSH_BUFFER)
            break; // Wtf. TODO

        agent_push_buffer_data_t processing_request;
        received = recv(agent->socket, &processing_request, sizeof(processing_request), 0);
        if (received == 0)
            break;

        server_processed_buffer_data_t processing_response = {};

        size_t size = 0;
        bool eof = false;

        int input_buf_handle = -1, output_buf_handle = -1;
        char *input_buffer = NULL, *output_buffer = NULL;
        size_t output_size;

        // Determine where to load the input from
        if (processing_request.size > 0) {
            // Perform action on agent-provided input data
            input_buffer = agent->input_buffer;
            size = processing_request.size;
            eof = processing_request.eof;
        } else {
            // Wait for the results of the preceding AFU
            pthread_mutex_lock(agent->internal_input_mutex);
            pthread_cond_wait(agent->internal_input_condition, agent->internal_input_mutex);

            input_buf_handle = agent->internal_input_buffer_handle;
            size = agent->internal_input_size; // TODO: is this correct?
            eof = agent->internal_input_eof;

            pthread_mutex_unlock(agent->internal_input_mutex);
        }

        // Determine where to put the output
        if (agent->output_agent == NULL) {
            // Copy to host

            // If we haven't yet established an output buffer for the agent, do it now
            if (!agent->output_buffer) {
                message_type_t output_buffer_message_type = SERVER_INITIALIZE_OUTPUT_BUFFER;
                server_initialize_output_buffer_data_t output_buffer_message;

                create_temp_file_for_shared_buffer(
                    output_buffer_message.output_buffer_filename,
                    sizeof(output_buffer_message.output_buffer_filename),
                    &agent->output_file, &agent->output_buffer);

                send(agent->socket, &output_buffer_message_type, sizeof(output_buffer_message_type), 0);
                send(agent->socket, &output_buffer_message, sizeof(output_buffer_message), 0);
            }

            output_buffer = agent->output_buffer;
        } else {
            output_buf_handle = create_new_buffer();
        }

        perform_afu_action(agent->afu_type,
            input_buf_handle, input_buffer, size,
            output_buf_handle, output_buffer, &output_size);

        if (output_buffer) {
            processing_response.size = output_size;
            processing_response.eof = eof;
            message_type = SERVER_PROCESSED_BUFFER;
            send(agent->socket, &message_type, sizeof(message_type), 0);
            send(agent->socket, &processing_response, sizeof(processing_response), 0);
        } else {
            pthread_mutex_lock(agent->output_agent->internal_input_mutex);
            agent->output_agent->internal_input_buffer_handle = output_buf_handle;
            agent->output_agent->internal_input_size = output_size;
            agent->output_agent->internal_input_eof = eof;
            pthread_cond_signal(agent->output_agent->internal_input_condition);
            pthread_mutex_unlock(agent->output_agent->internal_input_mutex);

            if (eof)
                send(agent->socket, &processing_response, sizeof(processing_response), 0);
        }

        if (eof) {
            break;
        }
    }

    {
        pthread_mutex_lock(&registered_agent_mutex);

        // Remove from list
        RemoveEntryList(&agent->Link);

        if (agent->input_agent) {
            agent->input_agent->output_agent = NULL;
        }

        if (agent->output_agent) {
            agent->output_agent->input_agent = NULL;
        }

        pthread_mutex_unlock(&registered_agent_mutex);
    }

    // Close stuff
    if (agent->input_buffer) {
        munmap(agent->input_buffer, BUFFER_SIZE);
        close(agent->input_file);
        agent->input_buffer = NULL, agent->input_file = -1;
    }

    if (agent->output_buffer) {
        munmap(agent->output_buffer, BUFFER_SIZE);
        close(agent->output_file);
        agent->output_buffer = NULL, agent->output_file = -1;
    }

    close(agent->socket);

    free(agent->internal_input_mutex);
    free(agent->internal_input_condition);
    free(agent);

    return 0;
}
