#include <malloc.h>
#include <include/gtest/gtest.h>
#include <metal_pipeline/operator_registry.hpp>
#include <metal_pipeline/data_source.hpp>
#include <metal_pipeline/data_sink.hpp>
#include <metal_fpga/hw/hls/include/action_metalfpga.h>
#include <metal_pipeline/pipeline_definition.hpp>
#include <metal_pipeline/pipeline_runner.hpp>
#include "base_test.hpp"


namespace metal {

static void fill_payload(uint8_t *buffer, uint64_t length) {
    for (uint64_t i = 0; i < length; ++i) {
        buffer[i] = i;
    }
}

//static void print_memory_64(void * mem)
//{
//    for (int i = 0; i < 64; ++i) {
//        printf("%02x ", ((uint8_t*)mem)[i]);
//        if (i%8 == 7) printf("\n");
//    }
//}

TEST_F(PipelineTest, ProfilingPipeline_ProfileOperators) {

    uint64_t n_pages = 1;

    uint64_t n_bytes = n_pages * 4096;
    auto *src = (uint8_t*)memalign(4096, n_bytes);
    fill_payload(src, n_bytes);

    auto *dest = (uint8_t*)memalign(4096, n_bytes);

    auto decrypt = _registry->operators().at("blowfish_decrypt");
    auto change_case = _registry->operators().at("change_case");
    auto encrypt = _registry->operators().at("blowfish_encrypt");

    auto keyBuffer = std::make_shared<std::vector<char>>(16);
    {
        std::vector<char> key ({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF });
        memcpy(keyBuffer->data(), key.data(), key.size());
    }

    decrypt->setOption("key", keyBuffer);
    encrypt->setOption("key", keyBuffer);

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

//    SnapAction action(METALFPGA_ACTION_TYPE, 0);

    auto pipeline = std::make_shared<PipelineDefinition>(std::vector<std::shared_ptr<AbstractOperator>> ({ dataSource, decrypt, change_case, encrypt, dataSink }));

    ProfilingPipelineRunner runner(pipeline, 0);

    runner.selectOperatorForProfiling(decrypt);
    ASSERT_NO_THROW(runner.run(true));
    runner.printProfilingResults();

    runner.selectOperatorForProfiling(change_case);
    ASSERT_NO_THROW(runner.run(true));
    runner.printProfilingResults();

    runner.selectOperatorForProfiling(encrypt);
    ASSERT_NO_THROW(runner.run(true));
    runner.printProfilingResults();
}

TEST_F(PipelineTest, ProfilingPipeline_BenchmarkChangecase) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 128;

    auto change_case = _registry->operators().at("change_case");
    auto dataSource = std::make_shared<RandomDataSource>(n_bytes);
    auto dataSink = std::make_shared<NullDataSink>(n_bytes);

    change_case->setOption("key", false);

    auto pipeline = std::make_shared<PipelineDefinition>(std::vector<std::shared_ptr<AbstractOperator>> ({ dataSource, change_case, dataSink }));

    ProfilingPipelineRunner runner(pipeline, 0);
    runner.selectOperatorForProfiling(change_case);
    ASSERT_NO_THROW(runner.run(true));
    runner.printProfilingResults();
}

} // namespace metal
