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
#include "../common/known_afus.h"
#include "list/list.h"
#include "afu.h"
#include "agent_worker.h"

LIST_ENTRY registered_agents;

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
            agent->argc = request.argc;
            agent->argv_len = request.argv_len;

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
                // Create mutexes for internal signaling
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
