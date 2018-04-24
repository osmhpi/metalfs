#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <pthread.h>

#include "../../metal/metal.h"

#include "../../metal_operators/op_read_file/operator.h"
#include "../../metal_operators/op_read_mem/operator.h"
#include "../../metal_operators/op_write_file/operator.h"
#include "../../metal_operators/op_write_mem/operator.h"
#include "../../metal_pipeline/pipeline.h"

#include "../common/buffer.h"
#include "../common/message.h"
#include "../common/known_operators.h"
#include "list/list.h"
#include "registered_agent.h"

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

uint64_t count_agents_with_afu_id(operator_id id) {
    uint64_t result = 0;

    if (IsListEmpty(&registered_agents))
        return result;

    PLIST_ENTRY current_link = registered_agents.Flink;
    do {
        registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

        if (memcmp(&current_agent->afu_type, &id, sizeof(operator_id)) == 0)
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
    memcpy(agent->cwd, request->cwd, FILENAME_MAX);

    uint64_t internal_filename_length = strlen(request->internal_input_filename);
    if (internal_filename_length) {
        agent->internal_input_file = malloc(internal_filename_length + 1);
        strncpy(agent->internal_input_file, request->internal_input_filename, internal_filename_length);
    }
    internal_filename_length = strlen(request->internal_output_filename);
    if (internal_filename_length) {
        agent->internal_output_file = malloc(internal_filename_length + 1);
        strncpy(agent->internal_output_file, request->internal_output_filename, internal_filename_length);
    }

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
    // except when we're writing to an internal file
    if (agent->input_agent_pid == 0 && !agent->internal_input_file) {
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
    }

    // Append to list
    InsertTailList(&registered_agents, &agent->Link);

    update_agent_wiring();
}

void send_agent_invalid(registered_agent_t *agent) {
    const char message[] = "An invalid operator chain was specified.\n";

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

    free(agent->internal_input_file);
    free(agent->internal_output_file);
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
                for (uint64_t i = 0; i < sizeof(known_operators) / sizeof(known_operators[0]); ++i) {
                    if (memcmp(&known_operators[i]->id, &current_agent->afu_type, sizeof(operator_id)) == 0) {
                        current_agent->afu_specification = known_operators[i];
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
            uint64_t current_operator = 0;
            do {
                registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);
                bool afu_valid = true;
                responses[current_operator] = current_agent->afu_specification->handle_opts(
                    current_agent->argc, current_agent->argv, response_lengths + current_operator, current_agent->cwd, &afu_valid);
                valid = valid && afu_valid;

                ++current_operator;

                current_link = current_link->Flink;
            } while (current_link != &pipeline_agents);

            // Send the responses and wait for incoming processing requests
            agent_push_buffer_data_t processing_request;
            current_link = pipeline_agents.Flink;
            current_operator = 0;
            do {
                registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

                message_type_t message_type = SERVER_ACCEPT_AGENT;
                send(current_agent->socket, &message_type, sizeof(message_type), 0);
                server_accept_agent_data_t accept_data = { response_lengths[current_operator], valid };
                send(current_agent->socket, &accept_data, sizeof(accept_data), 0);
                if (response_lengths[current_operator]) {
                    send(current_agent->socket, responses[current_operator], response_lengths[current_operator], 0);
                }

                message_type_t incoming_message_type;
                recv(current_agent->socket, &incoming_message_type, sizeof(incoming_message_type), 0);
                // Don't assert, see below, just have some faith
                // assert(incoming_message_type == AGENT_PUSH_BUFFER);

                // The only processing request that we really care about is the input agent's
                agent_push_buffer_data_t tmp_processing_request;
                recv(current_agent->socket, current_operator == 0 ? &processing_request : &tmp_processing_request,
                    sizeof(processing_request), 0);

                ++current_operator;

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

            registered_agent_t *input_agent = CONTAINING_LIST_RECORD(pipeline_agents.Flink, registered_agent_t);
            registered_agent_t *output_agent = CONTAINING_LIST_RECORD(pipeline_agents.Blink, registered_agent_t);

            // Add implicit data sources and sinks if necessary
            if (!input_agent->afu_specification->is_data_source) {
                ++pipeline_length;
            }

            // We always need to add a data sink
            ++pipeline_length;

            // Create the execution plan and apply the configuration
            operator_id operator_list[pipeline_length];
            current_operator = 0;

            if (!input_agent->afu_specification->is_data_source) {
                if (input_agent->internal_input_file) {
                    operator_list[0] = op_read_file_specification.id;
                } else {
                    operator_list[0] = op_read_mem_specification.id;
                }
                ++current_operator;
            }

            current_link = pipeline_agents.Flink;
            do {
                registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

                if (!current_agent->afu_specification->is_data_source) {
                    mtl_configure_afu(current_agent->afu_specification);
                }
                operator_list[current_operator++] = current_agent->afu_type;

                current_link = current_link->Flink;
            } while (current_link != &pipeline_agents);

            if (output_agent->internal_output_file) {
                operator_list[current_operator] = op_write_file_specification.id;
            } else {
                operator_list[current_operator] = op_write_mem_specification.id;
            }

            mtl_operator_execution_plan execution_plan = { operator_list, pipeline_length };
            mtl_configure_pipeline(execution_plan);

            // If we're interacting with an FPGA file, we have to set up the extents initially.
            // The op_read_file does this internally, so no action required here.
            uint64_t internal_input_file_length = 0;
            if (input_agent->afu_specification->is_data_source) {
                // TODO: Rethink
                internal_input_file_length = input_agent->afu_specification->get_file_length();
            } else if (input_agent->internal_input_file) {
                // TODO: Catch if file does not exist
                mtl_prepare_storage_for_reading(input_agent->internal_input_file, &internal_input_file_length);
            }

            uint64_t internal_input_file_offset = 0;
            uint64_t internal_output_file_offset = 0;
            bool internal_output_file_initialized = false;

            for (;;) {
                server_processed_buffer_data_t processing_response = {};

                size_t size = 0;
                bool eof = false;

                size_t output_size = 0;

                if (internal_input_file_length) {
                    size = BUFFER_SIZE < internal_input_file_length - internal_input_file_offset ? BUFFER_SIZE : internal_input_file_length - internal_input_file_offset;
                    eof = internal_input_file_offset + size == internal_input_file_length;
                } else {
                    // Use input agent-provided input data
                    size = processing_request.size;
                    eof = processing_request.eof;
                }

                if (size) {
                    if (output_agent->internal_output_file) {
                        if (!internal_output_file_initialized) {
                            if (internal_input_file_length) {
                                // When reading from internal input file, we know upfront how big the output file will be
                                mtl_prepare_storage_for_writing(output_agent->internal_output_file, internal_input_file_length);
                                internal_output_file_initialized = true;
                            } else {
                                // Increment size otherwise
                                mtl_prepare_storage_for_writing(output_agent->internal_output_file, internal_output_file_offset + size);
                            }
                        }

                        op_write_file_set_buffer(internal_output_file_offset, size);
                        mtl_configure_afu(&op_write_mem_specification);
                    } else if (!output_agent->output_buffer) {
                        // If we haven't yet established an output buffer for the output agent, do it now
                        message_type_t output_buffer_message_type = SERVER_INITIALIZE_OUTPUT_BUFFER;
                        server_initialize_output_buffer_data_t output_buffer_message;

                        create_temp_file_for_shared_buffer(
                            output_buffer_message.output_buffer_filename,
                            sizeof(output_buffer_message.output_buffer_filename),
                            &output_agent->output_file, &output_agent->output_buffer);

                        send(output_agent->socket, &output_buffer_message_type, sizeof(output_buffer_message_type), 0);
                        send(output_agent->socket, &output_buffer_message, sizeof(output_buffer_message), 0);

                        // Configure write_mem AFU
                        op_write_mem_set_buffer(output_agent->output_buffer, size); // TODO size may be different from input size
                        mtl_configure_afu(&op_write_mem_specification);
                    }

                    if (internal_input_file_length) {
                        op_read_file_set_buffer(internal_input_file_offset, size);
                        mtl_configure_afu(&op_read_file_specification);
                    } else {
                        // Configure the read_mem AFU
                        op_read_mem_set_buffer(input_agent->input_buffer, size);
                        mtl_configure_afu(&op_read_mem_specification);
                    }

                    mtl_run_pipeline();

                    // TODO:
                    // output_size = afu_write_mem_get_written_bytes();
                    output_size = size; // This is fine for now because we always get out the same amount of bytes
                }

                message_type_t message_type = SERVER_PROCESSED_BUFFER;

                if (internal_input_file_length) {
                    internal_input_file_offset += size;
                } else {
                    // Tell the input agent that we've processed the data
                    if (input_agent != output_agent || output_agent->internal_output_file) {
                        processing_response.size = 0;
                        processing_response.eof = eof;
                        send(input_agent->socket, &message_type, sizeof(message_type), 0);
                        send(input_agent->socket, &processing_response, sizeof(processing_response), 0);
                    }
                }

                if (output_agent->internal_output_file) {
                    internal_output_file_offset += size;
                } else {
                    // Tell the output agent to take the processing result
                    processing_response.size = output_size;
                    processing_response.eof = eof;
                    send(output_agent->socket, &message_type, sizeof(message_type), 0);
                    send(output_agent->socket, &processing_response, sizeof(processing_response), 0);
                }

                if (eof) {
                    // Terminate all other agents, but don't send another message to input or output agents
                    // If we've already notified them above
                    current_link = pipeline_agents.Flink;
                    do {
                        registered_agent_t *current_agent = CONTAINING_LIST_RECORD(current_link, registered_agent_t);

                        if ((current_agent != input_agent || internal_input_file_length) &&
                            (current_agent != output_agent || output_agent->internal_output_file)) {
                            processing_response.size = 0;
                            processing_response.eof = eof;
                            send(current_agent->socket, &message_type, sizeof(message_type), 0);
                            send(current_agent->socket, &processing_response, sizeof(processing_response), 0);
                        }

                        current_link = current_link->Flink;
                    } while (current_link != &pipeline_agents);
                    break;
                }

                if (!output_agent->internal_output_file) {
                    // Wait until the output agent is ready
                    if (input_agent != output_agent || internal_input_file_length) {
                        message_type_t incoming_message_type;
                        recv(output_agent->socket, &incoming_message_type, sizeof(incoming_message_type), 0);
                        assert(incoming_message_type == AGENT_PUSH_BUFFER);
                        agent_push_buffer_data_t tmp_processing_request;
                        recv(output_agent->socket, &tmp_processing_request, sizeof(tmp_processing_request), 0);
                    }
                }

                if (!internal_input_file_length) {
                    // Wait until the input agent has sent the next chunk of data
                    {
                        // There should be a better way how we handle crashed/disconnected clients
                        // (a good idea might be to check the results of all send/recv calls)
                        // For now, we'll just check if the input agent is still alive at this point
                        message_type_t incoming_message_type;
                        int received = recv(input_agent->socket, &incoming_message_type, sizeof(incoming_message_type), 0);
                        if (received == 0) {
                            break;
                        }
                        // Don't assert -- if we exit abnormally the card/action is left in a bad state
                        // assert(incoming_message_type == AGENT_PUSH_BUFFER);
                        recv(input_agent->socket, &processing_request, sizeof(processing_request), 0);
                    }
                }
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
