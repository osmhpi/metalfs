#include "buffer.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>

int create_temp_file_for_shared_buffer(char* file_name, size_t file_name_size, int* file, void** buffer) {
    char output_file_name[] = "/tmp/accfs-mmap-XXXXXX";
    *file = mkstemp(output_file_name);
    // snprintf(file_name, file_name_size, "/tmp/%s", output_file_name);
    strncpy(file_name, output_file_name, file_name_size);

    // Extend the file to BUFFER_SIZE by writing a null-byte at the end
    lseek(*file, BUFFER_SIZE-1, SEEK_SET);
    write(*file, "", 1);

    // Map it
    *buffer = mmap(NULL, BUFFER_SIZE, PROT_WRITE, MAP_SHARED, *file, 0);

    return 0;
}

int map_shared_buffer_for_reading(char* file_name, int* file, void** buffer) {
    *file = open(file_name, O_RDWR /*O_RDONLY*/);
    if (*file == -1)
        perror("open()");

    *buffer = mmap(NULL, BUFFER_SIZE, PROT_WRITE, MAP_SHARED, *file, 0);
    if (*buffer == MAP_FAILED)
        perror("mmap()");

    return 0;
}
