#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <mntent.h>
#include <libgen.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>

#include "../common/afus.h"
#include "../common/buffer.h"
#include "../common/message.h"

int get_process_connected_to_stdin(char * buffer, size_t bufsiz, int *pid) {

    // Determine the inode number of the stdin file descriptor
    struct stat stdin_stats;
    fstat(0, &stdin_stats);

    // TODO: Check errno

    // Enumerate processes
    char stdout_filename[FILENAME_MAX];
    char stdout_pipename[FILENAME_MAX];
    char attached_process[FILENAME_MAX];
    DIR *d;
    struct dirent *dir;
    d = opendir("/proc");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            bool isProcessId = true;
            for (size_t i = 0; i < strlen(dir->d_name); ++i) {
                if (!isdigit(dir->d_name[i])) {
                    isProcessId = false;
                    break;
                }
            }

            if (!isProcessId) {
                continue;
            }

            snprintf(stdout_filename, FILENAME_MAX, "/proc/%s/fd/1", dir->d_name);

            *pid = atoi(dir->d_name);

            // Check if stdout of this process is the same as our stdin pipe
            size_t len = readlink(stdout_filename, stdout_pipename, FILENAME_MAX);
            if (strncmp(stdout_pipename, "pipe:", 5 > len ? len : 5) == 0 && len > sizeof("pipe:[]")) {
                int64_t inode = atoll(stdout_pipename + sizeof("pipe["));
                if ((uint64_t)inode == stdin_stats.st_ino) {
                    snprintf(attached_process, FILENAME_MAX, "/proc/%s/exe", dir->d_name);
                    len = readlink(attached_process, buffer, bufsiz-1);
                    buffer[len] = '\0';
                    // TODO: ...
                    break;
                }
            }
        }
        closedir(d);
    } else {
        // TODO...
    }

    return 0;
}

int get_mount_point_of_filesystem(char *path, char * result, size_t size) {
    struct mntent *ent;
    FILE *mountsFile;

    mountsFile = setmntent("/proc/mounts", "r");
    if (mountsFile == NULL) {
        perror("setmntent");
        return -1;
    }
    size_t previousLongestPath = 0;
    size_t pathlen = strlen(path);
    while (NULL != (ent = getmntent(mountsFile))) {
        size_t mntlen = strlen(ent->mnt_dir);
        if (mntlen > pathlen) {
            continue;
        }

        if (mntlen < previousLongestPath) {
            continue;
        }

        if (strncmp(path, ent->mnt_dir, mntlen) == 0) {
            strncpy(result, ent->mnt_dir, mntlen+1 > size ? size : mntlen+1);
            previousLongestPath = mntlen;
        }
    }

    endmntent(mountsFile);

    if (previousLongestPath == 0) {
        return -1;
    }

    return 0;
}

int determine_afu_key(char *filename) {
    char *base = basename(filename);
    for (size_t i = 0; i < sizeof(afus)/sizeof(afu_entry_t); ++i) {
        if (strcmp(base, afus[i].name) == 0) {
            return afus[i].key;
        }
    }

    return -1;
}

int main() {

    // Find out our own filename and fs mount point
    char own_file_name[FILENAME_MAX];
    size_t len = readlink("/proc/self/exe", own_file_name, FILENAME_MAX-1);
    own_file_name[len] = '\0';
    // TODO: ...

    int afu = determine_afu_key(own_file_name);

    char own_fs_mount_point[FILENAME_MAX];
    if (get_mount_point_of_filesystem(own_file_name, own_fs_mount_point, FILENAME_MAX))
        printf("oopsie\n");

    // Determine the file connected to stdin
    char stdin_file[FILENAME_MAX];
    len = readlink("/proc/self/fd/0", stdin_file, FILENAME_MAX-1);
    stdin_file[len] = '\0';

    // Check if we're reading from an FPGA file
    int stdin_file_len = strlen(stdin_file);
    char files_prefix[FILENAME_MAX];
    snprintf(files_prefix, FILENAME_MAX, "%s/files/", own_fs_mount_point);
    int files_prefix_len = strlen(own_fs_mount_point);
    char* internal_input_filename = "";
    if (strncmp(files_prefix, stdin_file, files_prefix_len > stdin_file_len ? stdin_file_len : files_prefix_len) == 0) {
        internal_input_filename = basename(stdin_file);
    }

    int input_pid = 0;
    if (strlen(internal_input_filename) == 0) {
        // Determine if the process that's talking to us is another AFU
        char stdin_executable[FILENAME_MAX] = { 0 };
        get_process_connected_to_stdin(stdin_executable, sizeof(stdin_executable), &input_pid);

        char stdin_executable_fs_mount_point[FILENAME_MAX];
        if (!strlen(stdin_executable) || get_mount_point_of_filesystem(stdin_executable, stdin_executable_fs_mount_point, FILENAME_MAX)) {
            // printf("whooopsie\n");
        }

        if (!strlen(stdin_executable) || strcmp(stdin_executable_fs_mount_point, own_fs_mount_point) != 0) {
            // Reset input_pid
            input_pid = 0;
        }
    }

    // Determine the file connected to stdout
    char stdout_file[FILENAME_MAX];
    len = readlink("/proc/self/fd/1", stdout_file, FILENAME_MAX-1);
    stdout_file[len] = '\0';

    // Check if we're writing to an FPGA file
    int stdout_file_len = strlen(stdout_file);
    char* internal_output_filename = "";
    if (strncmp(files_prefix, stdout_file, files_prefix_len > stdout_file_len ? stdout_file_len : files_prefix_len) == 0) {
        internal_output_filename = basename(stdout_file); 
    }

    // Say hello to the filesystem!
    char socket_filename[FILENAME_MAX];
    sprintf(socket_filename, "%s/.hello", own_fs_mount_point);

    int sock = 0;
    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, socket_filename);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        perror("socket() failed");

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        perror("connect() failed");

    int input_file = -1, output_file = -1;
    char *input_buffer = NULL, *output_buffer = NULL;
    size_t bytes_read = 0;
    bool eof = false;

    agent_hello_data_t request = {
        .pid = getpid(),
        .afu_type = afu,
        .input_agent_pid = input_pid
    };
    strncpy(request.internal_input_filename, internal_input_filename, sizeof(request.internal_input_filename));
    strncpy(request.internal_output_filename, internal_output_filename, sizeof(request.internal_output_filename));

    // If there's no agent connected to stdin, we have to create a memory-mapped file
    if (input_pid == 0) {
        create_temp_file_for_shared_buffer(
            request.input_buffer_filename,
            sizeof(request.input_buffer_filename),
            &input_file, &input_buffer);
    }

    message_type_t message_type = AGENT_HELLO;
    send(sock, &message_type, sizeof(message_type), 0);
    if (send(sock, &request, sizeof(request), 0) < 0)
        perror("send() failed");

    message_type_t incoming_message_type;
    size_t received = recv(sock, &incoming_message_type, sizeof(incoming_message_type), 0);
    if (received == 0)
        return -1;
    else if (received < 0)
        perror("recv() failed");

    if (incoming_message_type == SERVER_ACCEPT_AGENT) {
        // Process input
        for (;;) {
            if (input_buffer != NULL) {
                bytes_read = fread(input_buffer,  sizeof(char), BUFFER_SIZE, stdin);
                eof = bytes_read < BUFFER_SIZE && feof(stdin);
            }

            if (eof && input_buffer) {
                // Unmap buffer
                munmap(input_buffer, BUFFER_SIZE);
                close(input_file);
                input_buffer = NULL, input_file = -1;
            }

            // Tell the server about the data (if any)
            // and wait for it to be consumed
            message_type = AGENT_PUSH_BUFFER;
            agent_push_buffer_data_t processing_request = {
                .size = bytes_read,
                .eof = eof
            };
            send(sock, &message_type, sizeof(message_type), 0);
            send(sock, &processing_request, sizeof(processing_request), 0);

            size_t received = recv(sock, &incoming_message_type, sizeof(incoming_message_type), 0);
            if (received == 0)
                break;
            
            if (incoming_message_type == SERVER_INITIALIZE_OUTPUT_BUFFER) {
                server_initialize_output_buffer_data_t message;
                // Read data
                received = recv(sock, &message, sizeof(message), 0);
                if (received == 0)
                    break;

                // Should always be true - we expect this message type only once
                if (output_file == -1)
                    map_shared_buffer_for_reading(message.output_buffer_filename, &output_file, &output_buffer);

                // Wait for the next message
                received = recv(sock, &incoming_message_type, sizeof(incoming_message_type), 0);
                if (received == 0)
                    break;
            }

            if (incoming_message_type != SERVER_PROCESSED_BUFFER)
                break;

            server_processed_buffer_data_t processing_response;
            received = recv(sock, &processing_response, sizeof(processing_response), 0);

            if (received == 0)
                break;

            if (processing_response.size && output_buffer) {
                // Write to stdout
                fwrite(output_buffer, sizeof(char), processing_response.size, stdout);
            }

            eof = processing_response.eof;

            if (eof && output_buffer) {
                // Unmap buffer
                munmap(output_buffer, BUFFER_SIZE);
                close(output_file);
                output_buffer = NULL, output_file = -1;
            }

            if (eof)
                break;
        }
    }

    close(sock);

    return 0;
}
