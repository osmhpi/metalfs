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

TEST_F(BaseTest, Thesis_ProfileOperators) {

    uint64_t n_pages = 1;

    uint64_t n_bytes = n_pages * 4096;
    uint8_t *src = (uint8_t*)memalign(4096, n_bytes);
    fill_payload(src, n_bytes);

    uint8_t *dest = (uint8_t*)memalign(4096, n_bytes);

    operator_id operator_list[] = {
        op_read_mem_specification.id,
        op_blowfish_decrypt_specification.id,
        op_change_case_specification.id,
        op_blowfish_encrypt_specification.id,
        op_write_mem_specification.id
    };

    mtl_operator_execution_plan execution_plan = { operator_list, sizeof(operator_list) / sizeof(operator_list[0]) };
    mtl_configure_pipeline(execution_plan);

    unsigned char key[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };

    op_blowfish_encrypt_set_key(key);
    mtl_configure_operator(&op_blowfish_encrypt_specification);

    op_blowfish_decrypt_set_key(key);
    mtl_configure_operator(&op_blowfish_decrypt_specification);

    op_read_mem_set_buffer(src, n_bytes);
    mtl_configure_operator(&op_read_mem_specification);

    op_write_mem_set_buffer(dest, n_bytes);
    mtl_configure_operator(&op_write_mem_specification);

    char perfmon_results[4096];

    mtl_configure_perfmon(op_blowfish_decrypt_specification.id.stream_id, op_blowfish_decrypt_specification.id.stream_id);
    ASSERT_EQ(MTL_SUCCESS, mtl_run_pipeline());
    mtl_read_perfmon(perfmon_results, sizeof(perfmon_results) - 1);
    printf("%s\n", perfmon_results);

    mtl_configure_perfmon(op_change_case_specification.id.stream_id, op_change_case_specification.id.stream_id);
    ASSERT_EQ(MTL_SUCCESS, mtl_run_pipeline());
    mtl_read_perfmon(perfmon_results, sizeof(perfmon_results) - 1);
    printf("%s\n", perfmon_results);

    mtl_configure_perfmon(op_blowfish_encrypt_specification.id.stream_id, op_blowfish_encrypt_specification.id.stream_id);
    ASSERT_EQ(MTL_SUCCESS, mtl_run_pipeline());
    mtl_read_perfmon(perfmon_results, sizeof(perfmon_results) - 1);
    printf("%s\n", perfmon_results);
}

TEST_F(BaseTest, Thesis_BenchmarkChangecase) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 128;

    operator_id operator_list[] = { op_read_file_specification.id, op_change_case_specification.id, op_write_file_specification.id };

    mtl_operator_execution_plan execution_plan = { operator_list, sizeof(operator_list) / sizeof(operator_list[0]) };
    mtl_configure_pipeline(execution_plan);

    op_read_file_set_random(n_bytes);
    mtl_configure_operator(&op_read_file_specification);

    op_write_file_set_buffer(0, 0);  // Means: /dev/null
    mtl_configure_operator(&op_write_file_specification);

    mtl_configure_perfmon(op_change_case_specification.id.stream_id, op_change_case_specification.id.stream_id);

    ASSERT_EQ(MTL_SUCCESS, mtl_run_pipeline());

    char perfmon_results[4096];
    mtl_read_perfmon(perfmon_results, sizeof(perfmon_results) - 1);
    printf("%s\n", perfmon_results);
}

} // namespace
