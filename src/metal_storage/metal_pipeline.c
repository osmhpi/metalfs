#include "storage.h"

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

#include "../metal/metal.h"

#include "../metal_pipeline/pipeline.hpp"

#include "../metal_operators/op_read_file/operator.h"
#include "../metal_operators/op_read_mem/operator.h"
#include "../metal_operators/op_write_file/operator.h"
#include "../metal_operators/op_write_mem/operator.h"

int mtl_storage_initialize() {

    return mtl_pipeline_initialize();
}

int mtl_storage_deinitialize() {

    return mtl_pipeline_deinitialize();
}

int mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
    if (metadata) {
        metadata->num_blocks = 64 * 1024 * 1024;
        metadata->block_size = 4096;
    }

    return MTL_SUCCESS;
}

int mtl_storage_set_active_read_extent_list(const mtl_file_extent *extents, uint64_t length) {

    mtl_pipeline_set_file_read_extent_list(extents, length);
    return MTL_SUCCESS;
}

int mtl_storage_set_active_write_extent_list(const mtl_file_extent *extents, uint64_t length) {

    mtl_pipeline_set_file_write_extent_list(extents, length);
    return MTL_SUCCESS;
}

int mtl_storage_read(uint64_t offset, void *buffer, uint64_t length) {

    void *aligned_buffer = snap_malloc(SNAP_ROUND_UP(length, 4096));

    // Build and run a pipeline
    operator_id operator_list[] = { op_read_file_specification.id, op_write_mem_specification.id };

    mtl_operator_execution_plan execution_plan = { .operators=operator_list, .length=2 };
    mtl_configure_pipeline(execution_plan);

    op_read_file_set_buffer(offset, length);
    mtl_configure_operator(&op_read_file_specification);

    op_write_mem_set_buffer(aligned_buffer, length);
    mtl_configure_operator(&op_write_mem_specification);

    mtl_run_pipeline();

    memcpy(buffer, aligned_buffer, length);
    free(aligned_buffer);

    return MTL_SUCCESS;
}

int mtl_storage_write(uint64_t offset, void *buffer, uint64_t length) {

    void *aligned_buffer = snap_malloc(length);
    memcpy(aligned_buffer, buffer, length);

    // Build and run a pipeline
    operator_id operator_list[] = { op_read_mem_specification.id, op_write_file_specification.id };

    mtl_operator_execution_plan execution_plan = { .operators=operator_list, .length=2 };
    mtl_configure_pipeline(execution_plan);

    op_read_mem_set_buffer(aligned_buffer, length);
    mtl_configure_operator(&op_read_mem_specification);

    op_write_file_set_buffer(offset, length);
    mtl_configure_operator(&op_write_file_specification);

    mtl_run_pipeline();

    mtl_finalize_operator(&op_write_file_specification);

    free(aligned_buffer);
    
    return MTL_SUCCESS;
}
