#pragma once

#include "../operators.h"

extern mtl_operator_specification op_blowfish_encrypt_specification;

void op_blowfish_encrypt_set_key(unsigned char key[16]);
