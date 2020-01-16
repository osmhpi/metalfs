#include <malloc.h>

#include <gtest/gtest.h>

#include <snap_action_metal.h>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/operator_registry.hpp>
#include <metal-pipeline/pipeline_definition.hpp>
#include <metal-pipeline/snap_action.hpp>

#include "base_test.hpp"

namespace metal {

using ColorfilterPipeline = SimulationPipelineTest;

TEST_F(ColorfilterPipeline, PreservesHeaderData) {
  uint64_t n_pages = 1;
  uint64_t n_bytes = n_pages * 4096;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  // The header spans the first 138 bytes. Set the rest to zeroes,
  // so the output will equal the input
  memset(src, 0, n_bytes);
  fill_payload(src, 138);

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  auto filter = try_get_operator("colorfilter");
  if (!filter) {
    GTEST_SKIP();
    return;
  }

  SnapAction action(fpga::ActionType, 0);

  auto pipeline = PipelineDefinition(std::move(*filter));
  pipeline.run(DataSource(src, n_bytes), DataSink(dest, n_bytes), action);

  EXPECT_EQ(0, memcmp(src, dest, n_bytes));

  free(src);
  free(dest);
}

}  // namespace metal
