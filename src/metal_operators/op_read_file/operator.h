#pragma once

#include "../../metal_storage/storage.h"

#include "../operators.h"

extern mtl_operator_specification op_read_file_specification;

void op_read_file_set_buffer(uint64_t offset, uint64_t length);
