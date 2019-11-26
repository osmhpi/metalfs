#include <gtest/gtest.h>
#include <malloc.h>
#include <metal_pipeline/operator_registry.hpp>
#include <metal_pipeline/pipeline_definition.hpp>
#include <snap_action_metal.h>
#include <metal_pipeline/data_sink.hpp>
#include <metal_pipeline/data_source.hpp>
#include <metal_pipeline/snap_action.hpp>
#include "base_test.hpp"

namespace metal {

using BlowfishPipeline = PipelineTest;

TEST_F(BlowfishPipeline, EncryptsAndDecryptsPayload) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    auto encrypt = try_get_operator("blowfish_encrypt");
    auto decrypt = try_get_operator("blowfish_decrypt");
    if (!encrypt || !decrypt) {
        GTEST_SKIP();
        return;
    }
    
    auto keyBuffer = std::make_shared<std::vector<char>>(16);
    {
        std::vector<char> key ({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF });
        memcpy(keyBuffer->data(), key.data(), key.size());
    }

    encrypt->setOption("key", keyBuffer);
    decrypt->setOption("key", keyBuffer);

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, encrypt, decrypt, dataSink });
    pipeline.run(action);

    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}


TEST_F(BlowfishPipeline, EncryptsAndDecryptsPayloadUsingDifferentKeys) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    auto encrypt = try_get_operator("blowfish_encrypt");
    auto decrypt = try_get_operator("blowfish_decrypt");
    if (!encrypt || !decrypt) {
        GTEST_SKIP();
        return;
    }

    auto encryptKeyBuffer = std::make_shared<std::vector<char>>(16);
    auto decryptKeyBuffer = std::make_shared<std::vector<char>>(16);
    {
        std::vector<char> encryptKey ({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF });
        memcpy(encryptKeyBuffer->data(), encryptKey.data(), encryptKey.size());

        std::vector<char> decryptKey ({ 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 });
        memcpy(decryptKeyBuffer->data(), decryptKey.data(), decryptKey.size());
    }

    encrypt->setOption("key", encryptKeyBuffer);
    decrypt->setOption("key", decryptKeyBuffer);

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, encrypt, decrypt, dataSink });
    pipeline.run(action);

    EXPECT_NE(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

} // namespace metal
