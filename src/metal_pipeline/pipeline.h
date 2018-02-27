#pragma once

#include "../metal_afus/afus.h"

void mtl_configure_afu(mtl_afu_specification *afu_spec);
void mtl_configure_pipeline(mtl_afu_execution_plan execution_plan);
void mtl_run_pipeline();
