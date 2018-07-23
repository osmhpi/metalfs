#include <malloc.h>

#include <metal/metal.h>
#include <metal_pipeline/pipeline.h>
#include <metal_operators/all_operators.h>

#include "base_test.hpp"

namespace {

void fill_payload(uint8_t *buffer, uint64_t length) {
    for (uint64_t i; i < length; ++i) {
        buffer[i] = i % 256;
    }
}

TEST_F(BaseTest, BlowfishPipeline_EncryptsAndDecryptsPayload) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    uint8_t *src = (uint8_t*)memalign(4096, n_bytes);
    fill_payload(src, n_bytes);

    uint8_t *dest = (uint8_t*)memalign(4096, n_bytes);

    operator_id operator_list[] = { op_read_mem_specification.id, op_blowfish_encrypt_specification.id, op_blowfish_decrypt_specification.id, op_write_mem_specification.id };

    mtl_operator_execution_plan execution_plan = { operator_list, sizeof(operator_list) / sizeof(operator_list[0]) };
    mtl_configure_pipeline(execution_plan);

    op_read_mem_set_buffer(src, n_bytes);
    mtl_configure_operator(&op_read_mem_specification);

    unsigned char key[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };

    op_blowfish_encrypt_set_key(key);
    mtl_configure_operator(&op_blowfish_encrypt_specification);

    op_blowfish_decrypt_set_key(key);
    mtl_configure_operator(&op_blowfish_decrypt_specification);

    op_write_mem_set_buffer(dest, n_bytes);
    mtl_configure_operator(&op_write_mem_specification);

    mtl_run_pipeline();

    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}


TEST_F(BaseTest, BlowfishPipeline_EncryptsAndDecryptsPayloadUsingDifferentKeys) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    uint8_t *src = (uint8_t*)memalign(4096, n_bytes);
    fill_payload(src, n_bytes);

    uint8_t *dest = (uint8_t*)memalign(4096, n_bytes);

    operator_id operator_list[] = { op_read_mem_specification.id, op_blowfish_encrypt_specification.id, op_blowfish_decrypt_specification.id, op_write_mem_specification.id };

    mtl_operator_execution_plan execution_plan = { operator_list, sizeof(operator_list) / sizeof(operator_list[0]) };
    mtl_configure_pipeline(execution_plan);

    op_read_mem_set_buffer(src, n_bytes);
    mtl_configure_operator(&op_read_mem_specification);

    unsigned char encrypt_key[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };
    op_blowfish_encrypt_set_key(encrypt_key);
    mtl_configure_operator(&op_blowfish_encrypt_specification);

	unsigned char decrypt_key[] = { 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
    op_blowfish_decrypt_set_key(decrypt_key);
    mtl_configure_operator(&op_blowfish_decrypt_specification);

    op_write_mem_set_buffer(dest, n_bytes);
    mtl_configure_operator(&op_write_mem_specification);

    mtl_run_pipeline();

    EXPECT_NE(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

} // namespace
