#include "known_afus.h"

#include "../../metal_afus/afu_passthrough/afu.h"
#include "../../metal_afus/afu_change_case/afu.h"

mtl_afu_specification* known_afus[] = {
    &afu_passthrough_specification,
    &afu_change_case_specification
};
