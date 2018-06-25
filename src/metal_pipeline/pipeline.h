#pragma once

#include "../metal_operators/operators.h"
#include "../metal_storage/storage.h"

int mtl_pipeline_initialize();
int mtl_pipeline_deinitialize();

void mtl_configure_operator(mtl_operator_specification *op_spec);
void mtl_finalize_operator(mtl_operator_specification *op_spec);
void mtl_configure_pipeline(mtl_operator_execution_plan execution_plan);
void mtl_run_pipeline();
void mtl_reset_perfmon();
void mtl_configure_perfmon(uint64_t stream_id0, uint64_t stream_id1);
uint64_t mtl_read_perfmon(char * buffer, uint64_t buffer_size);

void mtl_pipeline_set_file_read_extent_list(const mtl_file_extent *extents, uint64_t length);
void mtl_pipeline_set_file_write_extent_list(const mtl_file_extent *extents, uint64_t length);
