#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>

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

LIST_ENTRY registered_agents;

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

            message_t input_file_message = {
                .type = SERVER_ACCEPT_AGENT
            };

            // If the agent's input is not connected to another agent, respond
            // with a file that will be used for shared memory-mapping
            if (agent->input_agent_pid == 0) {
                char input_file_name[] = "accfs-mmap-XXXXXX";
                agent->input_file = mkstemp(input_file_name);
                strcpy(input_file_message.data.message, input_file_name);

                agent->input_buffer = mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_SHARED, agent->input_file, 0);
            }

            send(agent->socket, &input_file_message, sizeof(input_file_message), 0);

            // Append to list
            InsertTailList(&registered_agents, &agent->Link);
        } else {
            // Don't know this guy -- doesn't even say hello
            close(connfd);
        }
    }

    close(listenfd);

    return NULL;
}
