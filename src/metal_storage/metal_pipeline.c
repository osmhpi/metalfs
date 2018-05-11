#include "storage.h"

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../metal/metal.h"

#include "../metal_pipeline/pipeline.h"

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

    // Build and run a pipeline
    operator_id operator_list[] = { op_read_file_specification.id, op_write_mem_specification.id };

    mtl_operator_execution_plan execution_plan = { operator_list, 2 };
    mtl_configure_pipeline(execution_plan);

    op_read_file_set_buffer(offset, length);
    mtl_configure_afu(&op_read_file_specification);

    op_write_mem_set_buffer(buffer, length);
    mtl_configure_afu(&op_write_mem_specification);

    mtl_run_pipeline();

    return MTL_SUCCESS;
}

int mtl_storage_write(uint64_t offset, void *buffer, uint64_t length) {

    // Build and run a pipeline
    operator_id operator_list[] = { op_read_mem_specification.id, op_write_file_specification.id };

    mtl_operator_execution_plan execution_plan = { operator_list, 2 };
    mtl_configure_pipeline(execution_plan);

    op_read_mem_set_buffer(buffer, length);
    mtl_configure_afu(&op_read_mem_specification);

    op_write_file_set_buffer(offset, length);
    mtl_configure_afu(&op_write_file_specification);

    mtl_run_pipeline();

    mtl_finalize_afu(&op_write_file_specification);

    return MTL_SUCCESS;
}
