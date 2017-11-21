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

typedef struct registered_agent {
    int pid;
    int afu_type;
    int socket;

    struct registered_agent* input_agent;
    int input_agent_pid;
    int input_file;
    char* input_buffer;

    struct registered_agent* output_agent;
    int output_agent_pid;
    int output_file;
    char* output_buffer;

    LIST_ENTRY Link;
} registered_agent_t;

pthread_mutex_t registered_agent_mutex = PTHREAD_MUTEX_INITIALIZER;
LIST_ENTRY registered_agents;

void* agent_thread(void* args) {
    registered_agent_t *agent = args;

    message_t input_file_message = {
        .type = SERVER_ACCEPT_AGENT
    };

    send(agent->socket, &input_file_message, sizeof(input_file_message), 0);

    for (;;) {
        message_t processing_request;
        size_t received = recv(agent->socket, &processing_request, sizeof(processing_request), 0);
        if (received == 0)
            break;

        if (processing_request.type != AGENT_PUSH_BUFFER)
            break; // Wtf. TODO

        message_t processing_response = {
            .type = SERVER_PROCESSED_BUFFER
        };

        size_t size = processing_request.data.agent_push_buffer.size;
        bool eof = processing_request.data.agent_push_buffer.eof;

        if (size > 0) {
            // Do something. Would be good to forward data to the desired afu
            // in the future

            char demo_buffer[BUFFER_SIZE];
            memcpy(demo_buffer, agent->input_buffer, size);

            // Inspired by snap_education

            // Convert all char to lower case
            for (size_t i = 0; i < size; i++) {
                if (demo_buffer[i] >= 'A' && demo_buffer[i] <= 'Z')
                    demo_buffer[i] = demo_buffer[i] + ('a' - 'A');
                else
                    demo_buffer[i] = demo_buffer[i];
            }

            if (agent->output_agent != NULL) {
                // TODO: Write to internal buffer for the next agent and signal
                // that data has arrived
            } else {
                // If we haven't yet established an output buffer for the agent, do it now
                if (!agent->output_buffer) {
                    create_temp_file_for_shared_buffer(
                        processing_response.data.server_processed_buffer.output_buffer_filename,
                        sizeof(processing_response.data.server_processed_buffer.output_buffer_filename),
                        &agent->output_file, &agent->output_buffer);
                }

                memcpy(agent->output_buffer, demo_buffer, size);
                processing_response.data.server_processed_buffer.size = size;
                processing_response.data.server_processed_buffer.eof = eof;
            }
        } else {
            // TODO: Wait for the results of the preceding AFU
        }

        send(agent->socket, &processing_response, sizeof(processing_response), 0);

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
    free(agent);

    return 0;
}

void* start_socket(void* args) {
    InitializeListHead(&registered_agents);

    int listenfd = 0, connfd = 0;
    struct sockaddr_un serv_addr;

    char* name1 = (char*)args;
    printf("Socket %s\n", name1);

    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, name1);

    bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10); // 10 = Queue len

    for (;;) {
        connfd = accept(listenfd, NULL, NULL);

        message_t request;
        recv(connfd, &request, sizeof(request), 0);

        if (request.type == AGENT_HELLO) {
            registered_agent_t *agent = calloc(1, sizeof(registered_agent_t));
            agent->socket = connfd;
            agent->pid = request.data.agent_hello.pid;
            agent->afu_type = request.data.agent_hello.afu_type;
            agent->input_agent_pid = request.data.agent_hello.input_agent_pid;

            // If the agent's input is not connected to another agent, it should
            // have provided a file that will be used for memory-mapped data exchange
            if (agent->input_agent_pid == 0) {

                if (strlen(request.data.agent_hello.input_buffer_filename) == 0) {
                    // Should not happen
                    close(connfd);
                    free(agent);
                    continue;
                }

                map_shared_buffer_for_reading(
                    request.data.agent_hello.input_buffer_filename,
                    &agent->input_file, &agent->input_buffer
                );
            }

            {
                pthread_mutex_lock(&registered_agent_mutex);

                // Append to list
                InsertTailList(&registered_agents, &agent->Link);

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
