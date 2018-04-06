#pragma once

#include "../../metal_storage/storage.h"

#include "../operators.h"

extern mtl_operator_specification op_write_file_specification;

void op_write_file_set_extents(const mtl_file_extent *extents, uint64_t length);
