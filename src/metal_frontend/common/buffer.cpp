#include "buffer.hpp"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <cstdio>

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

int Buffer::map_shared_buffer_for_reading(std::string file_name, int *file, void **buffer) {
    *file = open(file_name, O_RDWR);
    if (*file == -1) {
        perror("open()");
        return -1;
    }

    *buffer = mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_SHARED, *file, 0);
    if (*buffer == MAP_FAILED) {
        perror("mmap()");
        close(*file);
        return -1;
    }

    return 0;
}

}
