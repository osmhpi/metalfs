#pragma once

#include <stddef.h>

#define BUFFER_SIZE (1024 * 8)

int create_temp_file_for_shared_buffer(char* file_name, size_t file_name_size, int* file, void** buffer);
int map_shared_buffer_for_reading(char* file_name, int* file, void** buffer);
