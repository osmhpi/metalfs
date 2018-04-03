#pragma once

#include "../metal_operators/operators.h"

int mtl_pipeline_initialize();
int mtl_pipeline_deinitialize();

void mtl_configure_afu(mtl_operator_specification *afu_spec);
void mtl_configure_pipeline(mtl_operator_execution_plan execution_plan);
void mtl_run_pipeline();
