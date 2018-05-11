#pragma once

#include "../metal_operators/operators.h"
#include "../metal_storage/storage.h"

int mtl_pipeline_initialize();
int mtl_pipeline_deinitialize();

void mtl_configure_afu(mtl_operator_specification *afu_spec);
void mtl_finalize_afu(mtl_operator_specification *afu_spec);
void mtl_configure_pipeline(mtl_operator_execution_plan execution_plan);
void mtl_run_pipeline();

void mtl_pipeline_set_file_read_extent_list(const mtl_file_extent *extents, uint64_t length);
void mtl_pipeline_set_file_write_extent_list(const mtl_file_extent *extents, uint64_t length);
