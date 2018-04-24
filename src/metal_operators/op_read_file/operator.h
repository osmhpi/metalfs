#pragma once

#include "../../metal_storage/storage.h"

#include "../operators.h"

extern mtl_operator_specification op_read_file_specification;

void op_read_file_set_buffer(uint64_t offset, uint64_t length);
void op_read_file_set_extents(const mtl_file_extent *extents, uint64_t length);
int op_read_file_load_extents_for_filename(const char* file);
