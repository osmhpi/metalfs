#include <ctype.h>
#include <dirent.h>
#include <libgen.h>
#include <mntent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/un.h>

#include <iostream>

#include <metal-driver-messages/buffer.hpp>
#include <metal-driver-messages/messages.hpp>
#include <metal-driver-messages/socket.hpp>

namespace metal {

struct ConnectedFile {
  std::string path;
  int pid;
  bool is_metal_file;
};

ConnectedFile get_process_connected_to_std_fd(int fd_no) {
  ConnectedFile result{};

  // Determine the inode number of the stdin file descriptor
  struct stat stdin_stats {};
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

      snprintf(stdout_filename, FILENAME_MAX, "/proc/%s/fd/%d", dir->d_name,
               1 - fd_no);

      // Check if stdout of this process is the same as our stdin pipe
      uint64_t len = readlink(stdout_filename, stdout_pipename, FILENAME_MAX);
      if (strncmp(stdout_pipename, "pipe:", 5 > len ? len : 5) == 0 &&
          len > sizeof("pipe:[]")) {
        int64_t inode = atoll(stdout_pipename + sizeof("pipe["));
        if ((uint64_t)inode == stdin_stats.st_ino) {
          snprintf(attached_process, FILENAME_MAX, "/proc/%s/exe", dir->d_name);

          char buffer[255];
          len = readlink(attached_process, buffer, sizeof(buffer) - 1);

          // TODO: ...

          buffer[len] = '\0';

          result = ConnectedFile{buffer, atoi(dir->d_name), false};
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

ConnectedFile determineProcessOrFileConnectedToStandardFileDescriptor(
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
    char files_prefix[FILENAME_MAX];  // TODO: Use format constant
    snprintf(files_prefix, FILENAME_MAX, "%s/files/", metal_mountpoint.c_str());
    auto files_prefix_len = strlen(files_prefix);

    char internal_filename[FILENAME_MAX] = {0};
    auto fd_file_len = strlen(fd_file);
    if (strncmp(files_prefix, fd_file,
                files_prefix_len > fd_file_len ? fd_file_len
                                               : files_prefix_len) == 0) {
      strncpy(internal_filename, fd_file + strlen(files_prefix) - 1,
              FILENAME_MAX);
    }

    if (strlen(internal_filename)) {
      return ConnectedFile{internal_filename, 0, true};
    }

    if (strncmp("/dev/null", fd_file, 9) == 0) {
      return ConnectedFile{"$NULL", 0, true};
    }

    return ConnectedFile{fd_file, 0, false};
  }

  // fd_file is probably the name of a pipe

  // Determine if the process that's talking to us is another operator
  ConnectedFile connected_process = get_process_connected_to_std_fd(fd_no);

  if (!connected_process.path.empty()) {
    char operators_prefix[FILENAME_MAX];  // TODO: Use format constant
    snprintf(operators_prefix, FILENAME_MAX, "%s/operators/",
             metal_mountpoint.c_str());
    auto operators_prefix_len = strlen(operators_prefix);

    // If this process is not another operator process, we reset the pid

    auto stdin_executable_len = strlen(connected_process.path.c_str());
    if (strncmp(operators_prefix, connected_process.path.c_str(),
                operators_prefix_len > stdin_executable_len
                    ? stdin_executable_len
                    : operators_prefix_len) != 0) {
      // Reset pid
      connected_process.pid = 0;
    }

    return connected_process;
  }

  return ConnectedFile{};
}

std::string getMountPointOfFilesystem(std::string path) {
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

std::string selfExecutableFilename() {
  // Find out our own filename and fs mount point
  char result[255];
  auto len = readlink("/proc/self/exe", result, sizeof(result) - 1);
  result[len] = '\0';
  return std::string(result);
}

std::string determineOperatorName() {
  // Find out our own filename and fs mount point
  char ownFileName[255];
  auto len = readlink("/proc/self/exe", ownFileName, sizeof(ownFileName) - 1);
  ownFileName[len] = '\0';

  char *base = basename(ownFileName);

  return std::string(base);
}

}  // namespace metal

int main(int argc, char *argv[]) {
  std::string operatorName = metal::determineOperatorName();

  std::string ownFSMountPoint =
      metal::getMountPointOfFilesystem(metal::selfExecutableFilename());

  // Determine the file connected to stdin
  auto input = metal::determineProcessOrFileConnectedToStandardFileDescriptor(
      0, ownFSMountPoint);

  // Determine the file connected to stdout
  auto output = metal::determineProcessOrFileConnectedToStandardFileDescriptor(
      1, ownFSMountPoint);

  // Say hello to the filesystem!
  char socketFileName[FILENAME_MAX];
  sprintf(socketFileName, "%s/.hello", ownFSMountPoint.c_str());

  int sock = 0;
  struct sockaddr_un serverAddress {};
  serverAddress.sun_family = AF_UNIX;
  strcpy(serverAddress.sun_path, socketFileName);

  if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("socket() failed");
  }

  if (connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) <
      0) {
    perror("connect() failed");
  }

  // Prepare to send the program parameters
  std::vector<uint64_t> argvLength(argc);
  uint64_t total_argv_len = 0;
  for (int i = 0; i < argc; ++i) {
    argvLength[i] = strlen(argv[i]) + 1;
    total_argv_len += argvLength[i];
  }
  std::vector<char> argv_buffer(total_argv_len);
  char *argv_cursor = (char *)argv_buffer.data();
  for (int i = 0; i < argc; ++i) {
    strcpy(argv_cursor, argv[i]);
    argv_cursor[argvLength[i] - 1] = '\0';
    argv_cursor += argvLength[i];
  }

  metal::RegistrationRequest request;
  request.set_pid(getpid());
  request.set_operator_type(operatorName);
  request.set_input_pid(input.pid);
  request.set_output_pid(output.pid);

  for (auto arg = 0; arg < argc; ++arg) {
    request.add_args(argv[arg]);
  }

  if (input.is_metal_file) request.set_metal_input_filename(input.path);
  if (output.is_metal_file) request.set_metal_output_filename(output.path);

  // Add the working directory to the request
  char cwd_buff[PATH_MAX];
  if (getcwd(cwd_buff, PATH_MAX) == nullptr) {
    throw std::runtime_error("Could not determine working directory.");
  }
  request.set_cwd(std::string(cwd_buff));
  request.set_metal_mountpoint(ownFSMountPoint);

  metal::Socket socket(sock);

  socket.sendMessage<metal::MessageType::RegistrationRequest>(request);

  auto response =
      socket.receiveMessage<metal::MessageType::RegistrationResponse>();

  if (response.has_error_msg()) {
    fprintf(stderr, "%s", response.error_msg().c_str());
  }

  if (!response.valid()) {
    return 1;
  }

  FILE *infd = stdin;

  std::optional<metal::Buffer> inputBuffer;
  std::optional<metal::Buffer> outputBuffer;

  if (response.has_input_buffer_filename()) {
    inputBuffer =
        metal::Buffer::mapSharedBuffer(response.input_buffer_filename(), true);
    if (response.has_agent_read_filename()) {
      infd = fopen(response.agent_read_filename().c_str(), "rb");
      // TODO: Better check for errors here...
    }
  }
  if (response.has_output_buffer_filename()) {
    outputBuffer = metal::Buffer::mapSharedBuffer(
        response.output_buffer_filename(), false);
  }

  uint64_t bytesRead = 0;
  bool eof = false;

  // Read the first input
  if (inputBuffer != std::nullopt) {
    bytesRead = fread(inputBuffer.value().current(), sizeof(char),
                       inputBuffer.value().size(), infd);
    eof = feof(infd) != 0;
    inputBuffer.value().swap();
  }

  metal::ProcessingResponse processingResponse;
  processingResponse.set_eof(false);

  // Processing loop
  while (true) {
    if (!processingResponse.eof()) {
      // Tell the server about the data (if any) and wait for it to be consumed
      metal::ProcessingRequest processingRequest;
      processingRequest.set_size(bytesRead);
      processingRequest.set_eof(eof);
      socket.sendMessage<metal::MessageType::ProcessingRequest>(
          processingRequest);
    }

    // Write output from previous iteration
    if (processingResponse.size() && outputBuffer != std::nullopt) {
      fwrite(outputBuffer.value().current(), sizeof(char),
             processingResponse.size(), stdout);
      outputBuffer.value().swap();
    }

    if (processingResponse.has_message()) {
      fprintf(stderr, "%s", processingResponse.message().c_str());
    }

    if (processingResponse.eof()) {
      break;
    }

    // Read the next input
    if (inputBuffer != std::nullopt && !eof) {
      bytesRead = fread(inputBuffer.value().current(), sizeof(char),
                         inputBuffer.value().size(), infd);
      eof = feof(infd) != 0;
      inputBuffer.value().swap();
    }

    // Wait for a server response
    try {
      processingResponse =
          socket.receiveMessage<metal::MessageType::ProcessingResponse>();
    } catch (std::exception &ex) {
      fprintf(stderr,
              "An error occurred during pipeline execution. Please check the "
              "filesystem driver logs.\n");
    }
  }

  if (infd != stdin) {
    fclose(infd);
  }

  return 0;
}
