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

#define BLOCK_SIZE 512
#define FILENAME_SIZE 512

int get_process_connected_to_stdin(char * buffer, size_t bufsiz, int *pid) {

    // Determine the inode number of the stdin file descriptor
    struct stat stdin_stats;
    fstat(0, &stdin_stats);

    // TODO: Check errno

    // Enumerate processes
    char stdout_filename[FILENAME_SIZE];
    char stdout_pipename[FILENAME_SIZE];
    char attached_process[FILENAME_SIZE];
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

            snprintf(stdout_filename, FILENAME_SIZE, "/proc/%s/fd/1", dir->d_name);

            *pid = atoi(dir->d_name);

            // Check if stdout of this process is the same as our stdin pipe
            size_t len = readlink(stdout_filename, stdout_pipename, FILENAME_SIZE);
            if (strncmp(stdout_pipename, "pipe:", 5 > len ? len : 5) == 0 && len > sizeof("pipe:[]")) {
                int64_t inode = atoll(stdout_pipename + sizeof("pipe["));
                if ((uint64_t)inode == stdin_stats.st_ino) {
                    snprintf(attached_process, FILENAME_SIZE, "/proc/%s/exe", dir->d_name);
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
    char own_file_name[FILENAME_SIZE];
    size_t len = readlink("/proc/self/exe", own_file_name, FILENAME_SIZE-1);
    own_file_name[len] = '\0';
    // TODO: ...

    int afu = determine_afu_key(own_file_name);

    char own_fs_mount_point[FILENAME_SIZE];
    if (get_mount_point_of_filesystem(own_file_name, own_fs_mount_point, FILENAME_SIZE))
        printf("oopsie\n");

    // Determine the file connected to stdin
    char stdin_file[FILENAME_SIZE];
    len = readlink("/proc/self/fd/0", stdin_file, FILENAME_SIZE-1);
    stdin_file[len] = '\0';

    // Check if we're reading from an FPGA file
    int stdin_file_len = strlen(stdin_file);
    int mountpoint_len = strlen(own_fs_mount_point);
    if (strncmp(own_fs_mount_point, stdin_file, mountpoint_len > stdin_file_len ? stdin_file_len : mountpoint_len) == 0) {
        fprintf(stderr, "Room for improvement\n");
    }

    // TODO: Only if we're not reading from an FPGA file
    // Determine if the process that's talking to us is another AFU
    int input_pid;
    char stdin_executable[FILENAME_SIZE] = { 0 };
    get_process_connected_to_stdin(stdin_executable, BLOCK_SIZE, &input_pid);
    if (strlen(stdin_executable))
        printf("Connected process executable: %s\n", stdin_executable);

    char stdin_executable_fs_mount_point[FILENAME_SIZE];
    if (!strlen(stdin_executable) || get_mount_point_of_filesystem(stdin_executable, stdin_executable_fs_mount_point, FILENAME_SIZE)) {
        // printf("whooopsie\n");
    }

    if (!strlen(stdin_executable) || strcmp(stdin_executable_fs_mount_point, own_fs_mount_point) != 0) {
        // printf("Processes live in different file systems!\n");
        input_pid = 0;
    } else {
        // printf("Both processes live under fs %s!\n", own_fs_mount_point);
    }

    // Determine the file connected to stdout
    char stdout_file[FILENAME_SIZE];
    len = readlink("/proc/self/fd/1", stdout_file, FILENAME_SIZE-1);
    stdout_file[len] = '\0';

    // Check if we're writing to an FPGA file
    int stdout_file_len = strlen(stdout_file);
    if (strncmp(own_fs_mount_point, stdout_file, mountpoint_len > stdout_file_len ? stdout_file_len : mountpoint_len) == 0) {
        fprintf(stderr, "More room for improvement\n");
    }

    // Say hello to the filesystem!
    char socket_filename[FILENAME_SIZE];
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

    message_t request = {
        .type = AGENT_HELLO,
        .data.agent_hello = {
            .pid = getpid(),
            .afu_type = afu,
            .input_agent_pid = input_pid
        }
    };

    // If there's no agent connected to stdin, we have create a memory-mapped file
    if (input_pid == 0) {
        create_temp_file_for_shared_buffer(
            request.data.agent_hello.input_buffer_filename,
            sizeof(request.data.agent_hello.input_buffer_filename),
            &input_file, &input_buffer);
    }

    if (send(sock, &request, sizeof(request), 0) < 0)
        perror("send() failed");

    message_t response;
    if (recv(sock, &response, sizeof(response), 0) < 0)
        perror("recv() failed");

    if (response.type == SERVER_ACCEPT_AGENT) {

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
            message_t processing_request = {
                .type = AGENT_PUSH_BUFFER,
                .data.agent_push_buffer = {
                    .size = bytes_read,
                    .eof = eof
                }
            };
            send(sock, &processing_request, sizeof(processing_request), 0);

            message_t processing_response;
            size_t received = recv(sock, &processing_response, sizeof(processing_response), 0);

            if (received == 0)
                break;

            if (processing_response.type != SERVER_PROCESSED_BUFFER)
                break;

            if (output_file == -1 &&
                strlen(processing_response.data.server_processed_buffer.output_buffer_filename)) {

                map_shared_buffer_for_reading(
                    processing_response.data.server_processed_buffer.output_buffer_filename,
                    &output_file, &output_buffer
                );
            }

            if (processing_response.data.server_processed_buffer.size && output_buffer) {
                // Write to stdout
                fwrite(output_buffer, sizeof(char), processing_response.data.server_processed_buffer.size, stdout);
            }

            eof = processing_response.data.server_processed_buffer.eof;

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
