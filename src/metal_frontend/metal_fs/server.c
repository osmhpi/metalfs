#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pthread.h>

#include "../../metal_afus/afu_read_mem/afu.h"
#include "../../metal_afus/afu_write_mem/afu.h"
#include "../../metal_pipeline/pipeline.h"

#include "../common/buffer.h"
#include "../common/message.h"
#include "../common/known_afus.h"
#include "list/list.h"
#include "agent_worker.h"

LIST_ENTRY registered_agents;
LIST_ENTRY pipeline_agents;

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

uint64_t count_agents_with_afu_id(afu_id id) {
    uint64_t result = 0;

    if (IsListEmpty(&registered_agents))
        return result;

    PLIST_ENTRY current_link = registered_agents.Flink;
    do {
        registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

        if (memcmp(&current_agent->afu_type, &id, sizeof(afu_id)) == 0)
            ++result;

        current_link = current_link->Flink;
    } while (current_link != &registered_agents);

    return result;
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

void derive_execution_plan() {
    // Looks for agents at the beginning of a pipeline
    // that have not been signaled yet

    if (IsListEmpty(&registered_agents))
        return;

    PLIST_ENTRY current_link = registered_agents.Flink;

    do {
        registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

        // Find the first agent of the pipeline (it should have an input that is not part of the pipeline)
        if (current_agent->input_agent_pid == 0) {
            // Walk the pipeline
            registered_agent_t *pipeline_agent = current_agent;
            uint64_t pipeline_length;
            while (pipeline_agent && pipeline_agent->output_agent_pid != 0) {
                pipeline_agent = pipeline_agent->output_agent;
                ++pipeline_length;
            }

            if (pipeline_agent) {
                // We have found an agent with output_agent_pid == 0, meaning it's an external file or pipe
                // So we have found a complete pipeline!
                // Now, remove all agents that are part of the pipeline from registered_agents and instead
                // insert them into pipeline_agents
                pipeline_agent = current_agent;
                while (pipeline_agent) {
                    RemoveEntryList(&pipeline_agent->Link);
                    InsertTailList(&pipeline_agents, &pipeline_agent->Link);

                    if (pipeline_agent->output_agent_pid == 0)
                        break;

                    pipeline_agent = pipeline_agent->output_agent;
                }
                break;
            }
        }

        current_link = current_link->Flink;
    } while (current_link != &registered_agents);
}

void register_agent(agent_hello_data_t *request, int connfd) {
    registered_agent_t *agent = calloc(1, sizeof(registered_agent_t));
    agent->socket = connfd;
    agent->pid = request->pid;
    agent->afu_type = request->afu_type;
    agent->input_agent_pid = request->input_agent_pid;
    agent->output_agent_pid = request->output_agent_pid;
    agent->argc = request->argc;

    // Read the agent's program args from the socket (if any)
    if (request->argc) {
        agent->argv_buffer = malloc(request->argv_len);
        recv(agent->socket, agent->argv_buffer, request->argv_len, 0);
        agent->argv = malloc(sizeof(char*) * request->argc);
        uint64_t current_arg = 0;
        void *argv_cursor = agent->argv_buffer;
        while (argv_cursor < agent->argv_buffer + request->argv_len) {
            agent->argv[current_arg++] = argv_cursor;
            argv_cursor += strlen(argv_cursor) + 1;
        }
    }

    // If the agent's input is not connected to another agent, it should
    // have provided a file that will be used for memory-mapped data exchange
    if (agent->input_agent_pid == 0) {
        if (strlen(request->input_buffer_filename) == 0) {
            // Should not happen
            close(connfd);
            free(agent);
            return;
        }

        map_shared_buffer_for_reading(
            request->input_buffer_filename,
            &agent->input_file, &agent->input_buffer
        );
    } else {
        // Create mutexes for internal signaling
        // agent->internal_input_buffer_handle = -1;
        // agent->internal_input_mutex = malloc(sizeof(pthread_mutex_t));
        // pthread_mutex_init(agent->internal_input_mutex, NULL);
        // agent->internal_input_condition = malloc(sizeof(pthread_cond_t));
        // pthread_cond_init(agent->internal_input_condition, NULL);
    }

    // Append to list
    InsertTailList(&registered_agents, &agent->Link);

    update_agent_wiring();
}

void send_agent_invalid(registered_agent_t *agent) {
    const char message[] = "An invalid AFU chain was specified.\n";

    message_type_t message_type = SERVER_ACCEPT_AGENT;
    send(agent->socket, &message_type, sizeof(message_type), 0);
    server_accept_agent_data_t accept_data = { sizeof(message), false };
    send(agent->socket, &accept_data, sizeof(accept_data), 0);
    send(agent->socket, message, sizeof(message), 0);
}

void free_registered_agent(registered_agent_t *agent) {
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
    free(agent->argv_buffer);
    free(agent->argv);
    free(agent);
}

void send_all_agents_invalid(PLIST_ENTRY agent_list) {
    while (!IsListEmpty(agent_list)) {
        registered_agent_t *free_agent = CONTAINING_LIST_RECORD(agent_list->Flink, registered_agent_t);
        send_agent_invalid(free_agent);
        RemoveEntryList(&free_agent->Link);
        free_registered_agent(free_agent);
    }
}

void* start_socket(void* args) {
    InitializeListHead(&registered_agents);
    InitializeListHead(&pipeline_agents);

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

            // Add the agent to the list of registered agents
            register_agent(&request, connfd);

            // Try to derive a full pipeline
            derive_execution_plan();
            if (IsListEmpty(&pipeline_agents)) {
                continue;
            }

            // Tell all remaining agents to go home
            send_all_agents_invalid(&registered_agents);

            // Check if each requested afu is used only once and look up the
            // AFU specification
            bool valid = true;
            uint64_t pipeline_length = 0;
            PLIST_ENTRY current_link = pipeline_agents.Flink;
            do {
                registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

                if (count_agents_with_afu_id(current_agent->afu_type) > 1) {
                    valid = false;
                    break;
                }

                // Look up the AFU specification that should be used
                for (uint64_t i = 0; i < sizeof(known_afus) / sizeof(known_afus[0]); ++i) {
                    if (memcmp(&known_afus[i]->id, &current_agent->afu_type, sizeof(afu_id)) == 0) {
                        current_agent->afu_specification = known_afus[i];
                        break;
                    }
                }

                if (!current_agent->afu_specification) {
                    valid = false;
                    break;
                }

                ++pipeline_length;

                current_link = current_link->Flink;
            } while (current_link != &pipeline_agents);

            if (!valid) {
                send_all_agents_invalid(&pipeline_agents);
                continue;
            }

            // Execute configuration step for each afu
            const char *responses[pipeline_length];
            uint64_t response_lengths[pipeline_length];
            current_link = pipeline_agents.Flink;
            uint64_t current_afu = 0;
            do {
                registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);
                bool afu_valid = true;
                responses[current_afu] = current_agent->afu_specification->handle_opts(
                    current_agent->argc, current_agent->argv, response_lengths + current_afu, &afu_valid);
                valid = valid && afu_valid;

                ++current_afu;

                current_link = current_link->Flink;
            } while (current_link != &pipeline_agents);

            // Send the responses and wait for incoming processing requests
            agent_push_buffer_data_t processing_request;
            current_link = pipeline_agents.Flink;
            current_afu = 0;
            do {
                registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

                message_type_t message_type = SERVER_ACCEPT_AGENT;
                send(current_agent->socket, &message_type, sizeof(message_type), 0);
                server_accept_agent_data_t accept_data = { response_lengths[current_afu], valid };
                send(current_agent->socket, &accept_data, sizeof(accept_data), 0);
                if (response_lengths[current_afu]) {
                    send(current_agent->socket, responses[current_afu], response_lengths[current_afu], 0);
                }

                // The only processing request that we really care about is the input agent's
                agent_push_buffer_data_t tmp_processing_request;
                recv(current_agent->socket, current_afu == 0 ? &processing_request : &tmp_processing_request,
                    sizeof(processing_request), 0);

                ++current_afu;

                current_link = current_link->Flink;
            } while (current_link != &pipeline_agents);

            if (!valid) {
                 while (!IsListEmpty(&pipeline_agents)) {
                    registered_agent_t *free_agent = CONTAINING_LIST_RECORD(pipeline_agents.Flink, registered_agent_t);
                    RemoveEntryList(&free_agent->Link);
                    free_registered_agent(free_agent);
                }
                continue;
            }

            // Apply the configuration, adding implicit read_mem and write_mem afus
            // This obviously needs to be adapted, once we support reading/writing from/to metal files
            afu_id afu_list[pipeline_length + 2];
            afu_list[0] = afu_read_mem_specification.id;
            current_link = pipeline_agents.Flink;
            current_afu = 1;
            do {
                registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

                mtl_configure_afu(current_agent->afu_specification);
                afu_list[current_afu++] = current_agent->afu_type;

                current_link = current_link->Flink;
            } while (current_link != &pipeline_agents);
            afu_list[current_afu] = afu_write_mem_specification.id;

            mtl_afu_execution_plan execution_plan = { afu_list, pipeline_length + 2 };
            mtl_configure_pipeline(execution_plan);

            registered_agent_t *input_agent = CONTAINING_LIST_RECORD(pipeline_agents.Flink, registered_agent_t);
            registered_agent_t *output_agent = CONTAINING_LIST_RECORD(pipeline_agents.Blink, registered_agent_t);

            for (;;) {
                message_type_t incoming_message_type;
                recv(input_agent->socket, &incoming_message_type, sizeof(incoming_message_type), 0);

                if (incoming_message_type != AGENT_PUSH_BUFFER)
                    break; // Should not happen

                server_processed_buffer_data_t processing_response = {};

                size_t size = 0;
                bool eof = false;

                size_t output_size = 0;

                // Use input agent-provided input data
                size = processing_request.size;
                eof = processing_request.eof;

                if (size) {
                    // If we haven't yet established an output buffer for the output agent, do it now
                    if (!output_agent->output_buffer) {
                        message_type_t output_buffer_message_type = SERVER_INITIALIZE_OUTPUT_BUFFER;
                        server_initialize_output_buffer_data_t output_buffer_message;

                        create_temp_file_for_shared_buffer(
                            output_buffer_message.output_buffer_filename,
                            sizeof(output_buffer_message.output_buffer_filename),
                            &output_agent->output_file, &output_agent->output_buffer);

                        send(output_agent->socket, &output_buffer_message_type, sizeof(output_buffer_message_type), 0);
                        send(output_agent->socket, &output_buffer_message, sizeof(output_buffer_message), 0);

                        // Configure write_mem AFU
                        afu_write_mem_set_buffer(output_agent->output_buffer, 4096);
                        mtl_configure_afu(&afu_write_mem_specification);
                    }

                    // Configure the read_mem AFU
                    afu_read_mem_set_buffer(input_agent->input_buffer, size);
                    mtl_configure_afu(&afu_read_mem_specification);

                    mtl_run_pipeline();

                    output_size = afu_write_mem_get_written_bytes();
                }

                message_type_t message_type = SERVER_PROCESSED_BUFFER;

                // Tell the input agent that we've processed the data
                if (input_agent != output_agent) {
                    processing_response.size = 0;
                    processing_response.eof = eof;
                    send(input_agent->socket, &message_type, sizeof(message_type), 0);
                    send(input_agent->socket, &processing_response, sizeof(processing_response), 0);
                }

                // Tell the output agent to take the processing result
                processing_response.size = output_size;
                processing_response.eof = eof;
                send(output_agent->socket, &message_type, sizeof(message_type), 0);
                send(output_agent->socket, &processing_response, sizeof(processing_response), 0);

                if (eof) {
                    // Terminate all other agents, too
                    current_link = pipeline_agents.Flink;
                    do {
                        registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

                        if (current_agent != input_agent && current_agent != output_agent) {
                            processing_response.size = 0;
                            processing_response.eof = eof;
                            send(current_agent->socket, &message_type, sizeof(message_type), 0);
                            send(current_agent->socket, &processing_response, sizeof(processing_response), 0);
                        }

                        current_link = current_link->Flink;
                    } while (current_link != &pipeline_agents);
                    break;
                }

                // Wait until the output agent is ready
                if (input_agent != output_agent) {
                    agent_push_buffer_data_t tmp_processing_request;
                    recv(output_agent->socket, &tmp_processing_request, sizeof(tmp_processing_request), 0);
                }

                // Wait until the input agent has sent the next chunk of data
                recv(input_agent->socket, &processing_request, sizeof(processing_request), 0);
            }

            // Clean up and close sockets
            while (!IsListEmpty(&pipeline_agents)) {
                registered_agent_t *free_agent = CONTAINING_LIST_RECORD(pipeline_agents.Flink, registered_agent_t);
                RemoveEntryList(&free_agent->Link);
                free_registered_agent(free_agent);
            }
        } else {
            // Don't know this guy -- doesn't even say hello
            close(connfd);
        }
    }

    close(listenfd);

    return NULL;
}
