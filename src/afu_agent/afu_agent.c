#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <mntent.h>
#include <libgen.h>
#include <fcntl.h>

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
                if (inode == stdin_stats.st_ino) {
                    snprintf(attached_process, FILENAME_SIZE, "/proc/%s/exe", dir->d_name);
                    readlink(attached_process, buffer, bufsiz);
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

    int input_pid;
    char test[FILENAME_SIZE] = { 0 };
    get_process_connected_to_stdin(test, BLOCK_SIZE, &input_pid);
    printf("Connected process executable: %s\n", test);

    char ownFilename[FILENAME_SIZE];
    readlink("/proc/self/exe", ownFilename, FILENAME_SIZE);
    // TODO: ...

    char connectedExecutableFsMountPoint[FILENAME_SIZE];
    if (get_mount_point_of_filesystem(test, connectedExecutableFsMountPoint, FILENAME_SIZE))
        printf("whooopsie\n");
    char ownExecutableFsMountPoint[FILENAME_SIZE];
    if (get_mount_point_of_filesystem(ownFilename, ownExecutableFsMountPoint, FILENAME_SIZE))
        printf("oopsie\n");

    if (strcmp(connectedExecutableFsMountPoint, ownExecutableFsMountPoint) != 0) {
        printf("Processes live in different file systems!\n%s\n%s\n",
            ownExecutableFsMountPoint, connectedExecutableFsMountPoint);
        input_pid = 0;
    } else {
        printf("Both processes live under fs %s!\n", ownExecutableFsMountPoint);
    }

    int afu = determine_afu_key(ownFilename);

    // Say hello to the filesystem!
    char socket_filename[FILENAME_SIZE];
    sprintf(socket_filename, "%s/.hello", ownExecutableFsMountPoint);

    int sock = 0;
    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, socket_filename);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        perror("socket() failed");

    if (connect(sock, &serv_addr, sizeof(serv_addr)) < 0)
        perror("connect() failed");

    message_t request = {
        .type = AGENT_HELLO,
        .data.agent_hello = {
            .pid = getpid(),
            .afu_type = afu,
            .input_agent_pid = input_pid
        }
    };

    if (send(sock, &request, sizeof(request), 0) < 0)
        perror("send() failed");

    message_t response;
    if (recv(sock, &response, sizeof(response), 0) < 0)
        perror("recv() failed");

    if (response.type == SERVER_ACCEPT_AGENT) {

        int input_file = -1, output_file = -1;
        char *input_buffer = NULL, *output_buffer = NULL;
        size_t bytes_read = 0;
        bool eof = false;

        // If there's no agent connected to stdin, we have to memory-map a file
        if (input_pid == 0) {
            if (strlen(response.data.message) == 0) {
                // Should not happen
                return 1;
            }

            input_file = open(response.data.message, O_WRONLY);
            input_buffer = mmap(NULL, BUFFER_SIZE, PROT_WRITE, MAP_SHARED, input_file, 0);
        }

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
            recv(sock, &processing_response, sizeof(processing_response), 0);

            if (processing_response.type != SERVER_PROCESSED_BUFFER)
                break;

            if (output_file == -1 &&
                strlen(processing_response.data.server_processed_buffer.output_buffer_filename)) {

                output_file = open(processing_response.data.server_processed_buffer.output_buffer_filename, O_RDONLY);
                output_buffer = mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_SHARED, output_file, 0);
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
