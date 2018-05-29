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

#include "../common/known_operators.h"
#include "../common/buffer.h"
#include "../common/message.h"

int get_process_connected_to_std_fd(int fd_no, char * buffer, size_t bufsiz, int *pid) {

    // Determine the inode number of the stdin file descriptor
    struct stat stdin_stats;
    fstat(fd_no, &stdin_stats);

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

            snprintf(stdout_filename, FILENAME_MAX, "/proc/%s/fd/%d", dir->d_name, 1-fd_no);

            // Check if stdout of this process is the same as our stdin pipe
            size_t len = readlink(stdout_filename, stdout_pipename, FILENAME_MAX);
            if (strncmp(stdout_pipename, "pipe:", 5 > len ? len : 5) == 0 && len > sizeof("pipe:[]")) {
                int64_t inode = atoll(stdout_pipename + sizeof("pipe["));
                if ((uint64_t)inode == stdin_stats.st_ino) {
                    snprintf(attached_process, FILENAME_MAX, "/proc/%s/exe", dir->d_name);
                    len = readlink(attached_process, buffer, bufsiz-1);
                    // TODO: ...
                    buffer[len] = '\0';
                    *pid = atoi(dir->d_name);
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

int determine_process_or_file_connected_to_std_fd (
    int fd_no, const char *metal_mountpoint, char *filename, size_t filename_size, bool *is_metal_file, int *pid) {

    char procfs_fd_file[256];
    snprintf(procfs_fd_file, sizeof(procfs_fd_file), "/proc/self/fd/%d", fd_no);

    // Determine the file connected to fd
    char fd_file[FILENAME_MAX];
    size_t len = readlink(procfs_fd_file, fd_file, FILENAME_MAX-1);
    fd_file[len] = '\0';

    // Is this an actual file?
    if (access(fd_file, F_OK) == 0) {
        // Check if it's an FPGA file
        char files_prefix[FILENAME_MAX]; // TODO: Use format constant
        snprintf(files_prefix, FILENAME_MAX, "%s/files/", metal_mountpoint);
        int files_prefix_len = strlen(files_prefix);

        char internal_filename [FILENAME_MAX] = {0};
        int fd_file_len = strlen(fd_file);
        if (strncmp(files_prefix, fd_file, files_prefix_len > fd_file_len ? fd_file_len : files_prefix_len) == 0) {
            strncpy(internal_filename, fd_file + strlen(files_prefix) - 1, FILENAME_MAX);
        }

        if (strlen(internal_filename)) {
            strncpy(filename, internal_filename, filename_size);
            *is_metal_file = true;
            *pid = 0;
            return 0;
        }

        strncpy(filename, fd_file, filename_size);
        *is_metal_file = false;
        *pid = 0;
        return 0;
    }

    // fd_file is probably the name of a pipe

    // Determine if the process that's talking to us is another operator
    char stdin_executable[FILENAME_MAX] = { 0 };
    get_process_connected_to_std_fd(fd_no, stdin_executable, sizeof(stdin_executable), pid);

    if (strlen(stdin_executable)) {
        char operators_prefix[FILENAME_MAX]; // TODO: Use format constant
        snprintf(operators_prefix, FILENAME_MAX, "%s/operators/", metal_mountpoint);
        int operators_prefix_len = strlen(operators_prefix);

        int stdin_executable_len = strlen(stdin_executable);
        if (strncmp(operators_prefix, stdin_executable, operators_prefix_len > stdin_executable_len ? stdin_executable_len : operators_prefix_len) != 0) {
            // Reset pid
            *pid = 0;
        }

        strncpy(filename, stdin_executable, filename_size);
        *is_metal_file = false;
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

operator_id determine_op_key(char *filename) {
    char *base = basename(filename);
    for (size_t i = 0; i < sizeof(known_operators); ++i) {
        if (strcmp(base, known_operators[i]->name) == 0) {
            return known_operators[i]->id;
        }
    }

    return known_operators[0]->id; // we should never get here
}

int main(int argc, char *argv[]) {

    // Find out our own filename and fs mount point
    char own_file_name[FILENAME_MAX];
    size_t len = readlink("/proc/self/exe", own_file_name, FILENAME_MAX-1);
    own_file_name[len] = '\0';
    // TODO: ...

    operator_id operator = determine_op_key(own_file_name);
    // TODO: Assert operator !~= (0, 0)

    char own_fs_mount_point[FILENAME_MAX];
    if (get_mount_point_of_filesystem(own_file_name, own_fs_mount_point, FILENAME_MAX))
        printf("oopsie\n");

    // Donate our current time slice, hoping that all other operator processes in the pipe are
    // spawned and recognizable afterwards. Seems to solve this race condition effectively.
    usleep(100);

    // Determine the file connected to stdin
    char stdin_file[FILENAME_MAX];
    bool stdin_is_metal_file;
    int stdin_pid;
    determine_process_or_file_connected_to_std_fd(
        0, own_fs_mount_point, stdin_file, sizeof(stdin_file), &stdin_is_metal_file, &stdin_pid);

    // Determine the file connected to stdout
    char stdout_file[FILENAME_MAX];
    bool stdout_is_metal_file;
    int stdout_pid;
    determine_process_or_file_connected_to_std_fd(
        1, own_fs_mount_point, stdout_file, sizeof(stdout_file), &stdout_is_metal_file, &stdout_pid);

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

    // Prepare to send the program parameters
    uint64_t argv_len[argc];
    uint64_t total_argv_len = 0;
    for (int i = 0; i < argc; ++i) {
        argv_len[i] = strlen(argv[i]) + 1;
        total_argv_len += argv_len[i];
    }
    char argv_buffer[total_argv_len];
    char *argv_cursor = (char*)argv_buffer;
    for (int i = 0; i < argc; ++i) {
        strcpy(argv_cursor, argv[i]);
        argv_cursor[argv_len[i] - 1] = '\0';
        argv_cursor += argv_len[i];
    }

    agent_hello_data_t request = {
        .pid = getpid(),
        .op_type = operator,
        .input_agent_pid = stdin_pid,
        .output_agent_pid = stdout_pid,
        .argc = argc,
        .argv_len = total_argv_len
    };
    if (stdin_is_metal_file)
        strncpy(request.internal_input_filename, stdin_file, sizeof(request.internal_input_filename));
    if (stdout_is_metal_file)
        strncpy(request.internal_output_filename, stdout_file, sizeof(request.internal_output_filename));

    // Add the working directory to the request
    getcwd(request.cwd, FILENAME_MAX);
    strncpy(request.metal_mountpoint, own_fs_mount_point, FILENAME_MAX);

    // If there's no agent connected to stdin, we have to create a memory-mapped file
    if (stdin_pid == 0) {
        create_temp_file_for_shared_buffer(
            request.input_buffer_filename,
            sizeof(request.input_buffer_filename),
            &input_file, &input_buffer);
    }

    message_type_t message_type = AGENT_HELLO;
    send(sock, &message_type, sizeof(message_type), 0);
    send(sock, &request, sizeof(request), 0);
    if (argc) {
        send(sock, &argv_buffer, total_argv_len, 0);
    }

    message_type_t incoming_message_type;
    int received = recv(sock, &incoming_message_type, sizeof(incoming_message_type), 0);
    if (received == 0)
        return -1;
    else if (received < 0)
        perror("recv() failed");

    if (incoming_message_type == SERVER_ACCEPT_AGENT) {
        server_accept_agent_data_t accept_data;
        recv(sock, &accept_data, sizeof(accept_data), 0);

        if (accept_data.response_length) {
            char response[accept_data.response_length];
            recv(sock, &response, accept_data.response_length, 0);
            fprintf(stderr, "%s", response);
        }

        // Process input
        while (accept_data.valid) {
            if (input_buffer != NULL) {
                bytes_read = fread(input_buffer, sizeof(char), BUFFER_SIZE, stdin);
                eof = feof(stdin);
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
