#include "known_operators.h"

#include "../../metal_operators/op_read_file/operator.h"
#include "../../metal_operators/op_passthrough/operator.h"
#include "../../metal_operators/op_change_case/operator.h"

mtl_operator_specification* known_operators[] = {
    &op_read_file_specification,
    &op_passthrough_specification,
    &op_change_case_specification
};
