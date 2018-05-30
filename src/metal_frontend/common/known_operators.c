#include "known_operators.h"

#include "../../metal_operators/op_read_file/operator.h"
#include "../../metal_operators/op_passthrough/operator.h"
#include "../../metal_operators/op_change_case/operator.h"
#include "../../metal_operators/op_blowfish_encrypt/operator.h"
#include "../../metal_operators/op_blowfish_decrypt/operator.h"

mtl_operator_specification* known_operators[] = {
    &op_read_file_specification,
    &op_passthrough_specification,
    &op_change_case_specification,
    &op_blowfish_encrypt_specification,
    &op_blowfish_decrypt_specification,
};
