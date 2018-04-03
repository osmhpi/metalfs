#pragma once

#include "../operators.h"

extern mtl_operator_specification op_read_mem_specification;

void op_read_mem_set_buffer(void *buffer, uint64_t length);
