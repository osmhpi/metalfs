#include <malloc.h>

#include <metal/metal.h>
#include <metal_pipeline/pipeline.h>
#include <metal_operators/all_operators.h>

#include "base_test.hpp"

namespace {

static void fill_payload(uint8_t *buffer, uint64_t length) {
    for (uint64_t i = 0; i < length; ++i) {
        buffer[i] = i;
    }
}

static void print_memory_64(void * mem)
{
    for (int i = 0; i < 64; ++i) {
        printf("%02x ", ((uint8_t*)mem)[i]);
        if (i%8 == 7) printf("\n");
    }
}

TEST_F(BaseTest, ReadWritePipeline_TransfersEntirePage) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    uint8_t *src = (uint8_t*)memalign(4096, n_bytes);
    fill_payload(src, n_bytes);

    uint8_t *dest = (uint8_t*)memalign(4096, n_bytes);

    operator_id operator_list[] = { op_read_mem_specification.id, op_write_mem_specification.id };

    mtl_operator_execution_plan execution_plan = { operator_list, sizeof(operator_list) / sizeof(operator_list[0]) };
    mtl_configure_pipeline(execution_plan);

    op_read_mem_set_buffer(src, n_bytes);
    mtl_configure_operator(&op_read_mem_specification);

    op_write_mem_set_buffer(dest, n_bytes);
    mtl_configure_operator(&op_write_mem_specification);

    mtl_run_pipeline();

    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

TEST_F(BaseTest, ReadWritePipeline_TransfersUnalignedDataSpanningMultiplePages) {

    uint64_t src_pages = 3;
    uint64_t src_bytes = src_pages * 4096;
    uint8_t *src = (uint8_t*)memalign(4096, src_bytes);

    uint64_t src_offset = 1815;
    uint64_t dest_offset = 2;
    uint64_t payload_bytes = 4711;

    fill_payload(src + src_offset, payload_bytes);

    uint64_t dest_pages = 2;
    uint64_t dest_bytes = dest_pages * 4096;
    uint8_t *dest = (uint8_t*)memalign(4096, dest_bytes);

    operator_id operator_list[] = { op_read_mem_specification.id, op_write_mem_specification.id };

    mtl_operator_execution_plan execution_plan = { operator_list, sizeof(operator_list) / sizeof(operator_list[0]) };
    mtl_configure_pipeline(execution_plan);

    op_read_mem_set_buffer(src + src_offset, payload_bytes);
    mtl_configure_operator(&op_read_mem_specification);

    op_write_mem_set_buffer(dest + dest_offset, payload_bytes);
    mtl_configure_operator(&op_write_mem_specification);

    mtl_run_pipeline();

    // print_memory_64(src + src_offset);
    // print_memory_64(dest + dest_offset);

    EXPECT_EQ(0, memcmp(src + src_offset, dest + dest_offset, payload_bytes));

    free(src);
    free(dest);
}

} // namespace
