#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <mntent.h>
#include <libgen.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>

#include <metal_frontend/messages/socket.hpp>
#include <metal_frontend/messages/buffer.hpp>
#include <iostream>
#include "Messages.pb.h"

namespace metal {

struct ConnectedFile {
  std::string path;
  int pid;
  bool is_metal_file;
};

ConnectedFile get_process_connected_to_std_fd(int fd_no) {

    ConnectedFile result {};

    // Determine the inode number of the stdin file descriptor
    struct stat stdin_stats{};
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
        while ((dir = readdir(d)) != nullptr) {
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

            snprintf(stdout_filename, FILENAME_MAX, "/proc/%s/fd/%d", dir->d_name, 1 - fd_no);

            // Check if stdout of this process is the same as our stdin pipe
            uint64_t len = readlink(stdout_filename, stdout_pipename, FILENAME_MAX);
            if (strncmp(stdout_pipename, "pipe:", 5 > len ? len : 5) == 0 && len > sizeof("pipe:[]")) {
                int64_t inode = atoll(stdout_pipename + sizeof("pipe["));
                if ((uint64_t) inode == stdin_stats.st_ino) {
                    snprintf(attached_process, FILENAME_MAX, "/proc/%s/exe", dir->d_name);

                    char buffer[255];
                    len = readlink(attached_process, buffer, sizeof(buffer) - 1);

                    // TODO: ...

                    buffer[len] = '\0';

                    result = ConnectedFile { buffer, atoi(dir->d_name), false };
                    break;
                }
            }
        }
        closedir(d);
    } else {
        // TODO...
    }

    return result;
}

ConnectedFile determine_process_or_file_connected_to_std_fd(
    int fd_no, const std::string metal_mountpoint) {

    char procfs_fd_file[256];
    snprintf(procfs_fd_file, sizeof(procfs_fd_file), "/proc/self/fd/%d", fd_no);

    // Determine the file connected to fd
    char fd_file[FILENAME_MAX];
    size_t len = readlink(procfs_fd_file, fd_file, FILENAME_MAX - 1);
    fd_file[len] = '\0';

    // Is this an actual file?
    if (access(fd_file, F_OK) == 0) {
        // Check if it's an FPGA file
        char files_prefix[FILENAME_MAX]; // TODO: Use format constant
        snprintf(files_prefix, FILENAME_MAX, "%s/files/", metal_mountpoint.c_str());
        auto files_prefix_len = strlen(files_prefix);

        char internal_filename[FILENAME_MAX] = {0};
        auto fd_file_len = strlen(fd_file);
        if (strncmp(files_prefix, fd_file, files_prefix_len > fd_file_len ? fd_file_len : files_prefix_len) == 0) {
            strncpy(internal_filename, fd_file + strlen(files_prefix) - 1, FILENAME_MAX);
        }

        if (strlen(internal_filename)) {
            return ConnectedFile { internal_filename, 0, true };
        }

//        if (strncmp("/dev/null", fd_file, 9) == 0) {
//            return ConnectedFile {};
//            *is_metal_file = true;
//            *pid = 0;
//            strncpy(filename, "$NULL", filename_size);
//            return 0;
//        }

        return ConnectedFile { fd_file, 0, false };
    }

    // fd_file is probably the name of a pipe

    // Determine if the process that's talking to us is another operator
    ConnectedFile connected_process = get_process_connected_to_std_fd(fd_no);

    if (!connected_process.path.empty()) {
        char operators_prefix[FILENAME_MAX]; // TODO: Use format constant
        snprintf(operators_prefix, FILENAME_MAX, "%s/operators/", metal_mountpoint.c_str());
        auto operators_prefix_len = strlen(operators_prefix);

        // If this process is not another operator process, we reset the pid

        auto stdin_executable_len = strlen(connected_process.path.c_str());
        if (strncmp(operators_prefix, connected_process.path.c_str(),
                    operators_prefix_len > stdin_executable_len ? stdin_executable_len : operators_prefix_len) != 0) {
            // Reset pid
            connected_process.pid = 0;
        }

        return connected_process;
    }

    return ConnectedFile{};
}

std::string get_mount_point_of_filesystem(std::string path) {
    struct mntent *ent;
    FILE *mountsFile;

    mountsFile = setmntent("/proc/mounts", "r");
    if (mountsFile == nullptr) {
        throw std::runtime_error("Could not read mounts");
    }

    std::string result;
    size_t previousLongestPath = 0;
    size_t pathlen = path.size();
    while (nullptr != (ent = getmntent(mountsFile))) {
        size_t mntlen = strlen(ent->mnt_dir);
        if (mntlen > pathlen) {
            continue;
        }

        if (mntlen < previousLongestPath) {
            continue;
        }

        if (strncmp(path.c_str(), ent->mnt_dir, mntlen) == 0) {
            result = std::string(ent->mnt_dir);
            previousLongestPath = mntlen;
        }
    }

    endmntent(mountsFile);

    if (previousLongestPath == 0) {
        throw std::runtime_error("Could not determine mount point");
    }

    return result;
}

std::string own_file_name() {
    // Find out our own filename and fs mount point
    char result[255];
    auto len = readlink("/proc/self/exe", result, sizeof(result) - 1);
    result[len] = '\0';
    return std::string(result);
}

std::string determine_op_key() {
    // Find out our own filename and fs mount point
    char own_file_name[255];
    auto len = readlink("/proc/self/exe", own_file_name, sizeof(own_file_name) - 1);
    own_file_name[len] = '\0';

    char *base = basename(own_file_name);

    return std::string(base);
}

} // namespace metal

int main(int argc, char *argv[]) {

    std::string operator_key = metal::determine_op_key();

    std::string own_fs_mount_point = metal::get_mount_point_of_filesystem(metal::own_file_name());

    // Determine the file connected to stdin
    auto input = metal::determine_process_or_file_connected_to_std_fd(
        0, own_fs_mount_point);

    // Determine the file connected to stdout
    auto output = metal::determine_process_or_file_connected_to_std_fd(
        1, own_fs_mount_point);

    // Say hello to the filesystem!
    char socket_filename[FILENAME_MAX];
    sprintf(socket_filename, "%s/.hello", own_fs_mount_point.c_str());

    int sock = 0;
    struct sockaddr_un serv_addr{};
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, socket_filename);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        perror("socket() failed");

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        perror("connect() failed");

//    int input_file = -1, output_file = -1;
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
    char *argv_cursor = (char *) argv_buffer;
    for (int i = 0; i < argc; ++i) {
        strcpy(argv_cursor, argv[i]);
        argv_cursor[argv_len[i] - 1] = '\0';
        argv_cursor += argv_len[i];
    }

    metal::ClientHello request;
    request.set_pid(getpid());
    request.set_operator_type(operator_key);
    request.set_input_pid(input.pid);
    request.set_output_pid(output.pid);

    for (auto arg = 0; arg < argc; ++arg) {
        request.add_args(argv[arg]);
    }

    if (input.is_metal_file)
        request.set_metal_input_filename(input.path);
    if (output.is_metal_file)
        request.set_metal_output_filename(output.path);

    // Add the working directory to the request
    char cwd_buff[PATH_MAX];
    if (getcwd(cwd_buff, PATH_MAX) == nullptr) {
        throw std::runtime_error("Could not determine working directory.");
    }
    request.set_cwd(std::string(cwd_buff));
    request.set_metal_mountpoint(own_fs_mount_point);

    metal::Socket socket(sock);

    socket.send_message<metal::message_type::AgentHello>(request);

    auto response = socket.receive_message<metal::message_type::ServerAcceptAgent>();

    if (response.has_error_msg()) {
        fprintf(stderr, "%s", response.error_msg().c_str());
    }

    if (!response.valid())
        return 1;

    std::optional<metal::Buffer> input_buffer;
    std::optional<metal::Buffer> output_buffer;

    if (response.has_input_buffer_filename()) {
        input_buffer = metal::Buffer::map_shared_buffer(response.input_buffer_filename(), true);
    }
    if (response.has_output_buffer_filename()) {
        output_buffer = metal::Buffer::map_shared_buffer(response.output_buffer_filename(), false);
    }

    // Process input
    while (true) {

        if (input_buffer != std::nullopt) {
            bytes_read = fread(input_buffer.value().buffer(), sizeof(char), input_buffer.value().size(), stdin);
            eof = feof(stdin) != 0;
        }

        // Tell the server about the data (if any) and wait for it to be consumed

        metal::ClientPushBuffer processing_request;
        processing_request.set_size(bytes_read);
        processing_request.set_eof(eof);
        socket.send_message<metal::message_type::AgentPushBuffer>(processing_request);
        auto processing_response = socket.receive_message<metal::message_type::ServerProcessedBuffer>();

        if (processing_response.size() && output_buffer != std::nullopt) {
            // Write to stdout
            fwrite(output_buffer.value().buffer(), sizeof(char), processing_response.size(), stdout);
        }

        if (processing_response.has_message()) {
            fprintf(stderr, "%s", processing_response.message().c_str());
        }

        if (processing_response.eof())
            break;
    }

    return 0;
}
