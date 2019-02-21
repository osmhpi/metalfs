#include "buffer.hpp"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <cstdio>
#include <stdexcept>

namespace metal {

Buffer Buffer::create_temp_file_for_shared_buffer() {
    char output_file_name[23] = "/tmp/accfs-mmap-XXXXXX";
    int file = mkstemp(output_file_name);

    // Extend the file to BUFFER_SIZE by writing a null-byte at the end
    lseek(file, BUFFER_SIZE - 1, SEEK_SET);
    write(file, "", 1);

    // Map it
    void *buffer = mmap(NULL, BUFFER_SIZE, PROT_WRITE, MAP_SHARED, file, 0);

    return Buffer(std::string(output_file_name), file, buffer);
}

Buffer Buffer::map_shared_buffer_for_reading(std::string file_name) {
    int file = open(file_name.c_str(), O_RDWR);
    if (file == -1) {
      throw std::runtime_error("Failed to open file");
    }

    void *buffer = mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_SHARED, file, 0);
    if (buffer == MAP_FAILED) {
      close(file);
      throw std::runtime_error("Failed to memory-map file");
    }

    return Buffer(file_name, file, buffer);
}

}
