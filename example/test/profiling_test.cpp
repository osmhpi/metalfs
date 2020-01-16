#include <gtest/gtest.h>
#include <malloc.h>
#include <snap_action_metal.h>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator_registry.hpp>
#include <metal-pipeline/pipeline_definition.hpp>
#include <metal-pipeline/pipeline_runner.hpp>
#include "base_test.hpp"

namespace metal {

using ProfilingPipeline = PipelineTest;

TEST_F(ProfilingPipeline, ProfileOperators) {
  uint64_t n_pages = 1;

  uint64_t n_bytes = n_pages * 4096;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  auto decrypt = try_get_operator("blowfish_decrypt");
  auto change_case = try_get_operator("changecase");
  auto encrypt = try_get_operator("blowfish_encrypt");
  if (!decrypt || !change_case || !encrypt) {
    GTEST_SKIP();
    return;
  }

  auto keyBuffer = std::make_shared<std::vector<char>>(16);
  {
    std::vector<char> key(
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF});
    memcpy(keyBuffer->data(), key.data(), key.size());
  }

  decrypt->setOption("key", keyBuffer);
  encrypt->setOption("key", keyBuffer);

  auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
  auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

  auto pipeline = std::make_shared<PipelineDefinition>(
      std::vector<std::shared_ptr<AbstractOperator>>(
          {dataSource, decrypt, change_case, encrypt, dataSink}));

  ProfilingPipelineRunner runner(pipeline, 0);

  runner.selectOperatorForProfiling(decrypt);
  ASSERT_NO_THROW(runner.run(true));
  //    std::cout << runner.formatProfilingResults();

  runner.selectOperatorForProfiling(change_case);
  ASSERT_NO_THROW(runner.run(true));
  //    std::cout << runner.formatProfilingResults();

  runner.selectOperatorForProfiling(encrypt);
  ASSERT_NO_THROW(runner.run(true));
  //    std::cout << runner.formatProfilingResults();
}

TEST_F(ProfilingPipeline, BenchmarkChangecase) {
  uint64_t n_pages = 1;
  uint64_t n_bytes = n_pages * 128;

  auto change_case = try_get_operator("changecase");
  if (!change_case) {
    GTEST_SKIP();
    return;
  }

  auto dataSource = std::make_shared<RandomDataSource>(n_bytes);
  auto dataSink = std::make_shared<NullDataSink>(n_bytes);

  change_case->setOption("lowercase", false);

  auto pipeline = std::make_shared<PipelineDefinition>(
      std::vector<std::shared_ptr<AbstractOperator>>(
          {dataSource, change_case, dataSink}));

  ProfilingPipelineRunner runner(pipeline, 0);
  runner.selectOperatorForProfiling(change_case);
  ASSERT_NO_THROW(runner.run(true));
  //    std::cout << runner.formatProfilingResults();
}

}  // namespace metal
