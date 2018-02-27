#pragma once

#include "../afus.h"

extern mtl_afu_specification afu_write_mem_specification;

void afu_write_mem_set_buffer(void *buffer);
uint64_t afu_write_mem_get_written_bytes();
