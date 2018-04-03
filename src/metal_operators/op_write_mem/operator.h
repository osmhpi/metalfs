#pragma once

#include "../operators.h"

extern mtl_operator_specification op_write_mem_specification;

void op_write_mem_set_buffer(void *buffer, uint64_t length);
uint64_t op_write_mem_get_written_bytes();
