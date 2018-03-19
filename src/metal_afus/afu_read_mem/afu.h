#pragma once

#include "../afus.h"

extern mtl_afu_specification afu_read_mem_specification;

void afu_read_mem_set_buffer(void *buffer, uint64_t length);
