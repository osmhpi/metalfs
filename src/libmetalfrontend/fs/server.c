#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pthread.h>

#include "../common/buffer.h"
#include "../common/message.h"
#include "list/list.h"
#include "afu.h"

typedef struct registered_agent {
    int pid;
    int afu_type;
    int socket;

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

pthread_mutex_t registered_agent_mutex = PTHREAD_MUTEX_INITIALIZER;
LIST_ENTRY registered_agents;

void* agent_thread(void* args) {
    registered_agent_t *agent = args;

    message_type_t message_type = SERVER_ACCEPT_AGENT;

    send(agent->socket, &message_type, sizeof(message_type), 0);

    for (;;) {
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
            size = agent->internal_input_size;
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

registered_agent_t* find_agent_with_pid(int pid) {
    if (IsListEmpty(&registered_agents))
        return NULL;

    PLIST_ENTRY current_link = registered_agents.Flink;
    do {
        registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

        if (current_agent->pid == pid)
            return current_agent;

        current_link = current_link->Flink;
    } while (current_link != &registered_agents);

    return NULL;
}

void update_agent_wiring() {
    if (IsListEmpty(&registered_agents))
        return;

    PLIST_ENTRY current_link = registered_agents.Flink;

    do {
        registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

        if (current_agent->input_agent_pid && current_agent->input_agent == NULL) {
            current_agent->input_agent = find_agent_with_pid(current_agent->input_agent_pid);
            if (current_agent->input_agent != NULL)
                current_agent->input_agent->output_agent = current_agent;
        }

        current_link = current_link->Flink;
    } while (current_link != &registered_agents);
}

void* start_socket(void* args) {
    InitializeListHead(&registered_agents);

    int listenfd = 0, connfd = 0;
    struct sockaddr_un serv_addr;

    char* socket_file_name = (char*)args;

    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, socket_file_name);

    bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10); // 10 = Queue len

    for (;;) {
        connfd = accept(listenfd, NULL, NULL);

        message_type_t incoming_message_type;
        recv(connfd, &incoming_message_type, sizeof(incoming_message_type), 0);

        if (incoming_message_type == AGENT_HELLO) {
            agent_hello_data_t request;
            recv(connfd, &request, sizeof(request), 0);

            registered_agent_t *agent = calloc(1, sizeof(registered_agent_t));
            agent->socket = connfd;
            agent->pid = request.pid;
            agent->afu_type = request.afu_type;
            agent->input_agent_pid = request.input_agent_pid;

            // If the agent's input is not connected to another agent, it should
            // have provided a file that will be used for memory-mapped data exchange
            if (agent->input_agent_pid == 0) {

                if (strlen(request.input_buffer_filename) == 0) {
                    // Should not happen
                    close(connfd);
                    free(agent);
                    continue;
                }

                map_shared_buffer_for_reading(
                    request.input_buffer_filename,
                    &agent->input_file, &agent->input_buffer
                );
            } else {
                // Create a mutexes for internal signaling
                agent->internal_input_buffer_handle = -1;
                agent->internal_input_mutex = malloc(sizeof(pthread_mutex_t));
                pthread_mutex_init(agent->internal_input_mutex, NULL);
                agent->internal_input_condition = malloc(sizeof(pthread_cond_t));
                pthread_cond_init(agent->internal_input_condition, NULL);
            }

            {
                pthread_mutex_lock(&registered_agent_mutex);

                // Append to list
                InsertTailList(&registered_agents, &agent->Link);

                update_agent_wiring();

                pthread_mutex_unlock(&registered_agent_mutex);
            }

            pthread_t agent_thr;
            pthread_create(&agent_thr, NULL, agent_thread, (void*)agent);
        } else {
            // Don't know this guy -- doesn't even say hello
            close(connfd);
        }
    }

    close(listenfd);

    return NULL;
}
