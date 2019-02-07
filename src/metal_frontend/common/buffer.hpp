#pragma once

#include <stddef.h>
#include <string>

#define BUFFER_SIZE (1024 * 128)

int create_temp_file_for_shared_buffer(std::string file_name, size_t file_name_size, int* file, void** buffer);
int map_shared_buffer_for_reading(std::string file_name, int* file, void** buffer);
