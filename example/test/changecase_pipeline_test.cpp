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

using ChangecasePipeline = SimulationPipelineTest;

TEST_F(ChangecasePipeline, TransformsToUppercase) {
  const char input[] = "Hello World";
  char dest[20] = {0};

  auto transformer = try_get_operator("changecase");
  if (!transformer) {
    // Could not find operator
    GTEST_SKIP();
    return;
  }

  transformer->setOption("lowercase", false);

  SnapAction action(fpga::ActionType, 0);

  auto pipeline = PipelineDefinition(std::move(*transformer));
  pipeline.run(DataSource(input, sizeof(input) - 1),
               DataSink(dest, sizeof(input) - 1), action);

  EXPECT_EQ("HELLO WORLD", std::string(dest));
}

TEST_F(ChangecasePipeline, TransformsToLowercase) {
  const char input[] = "Hello World";
  char dest[20] = {0};

  auto transformer = try_get_operator("changecase");
  if (!transformer) {
    // Could not find operator
    GTEST_SKIP();
    return;
  }

  transformer->setOption("lowercase", true);

  SnapAction action(fpga::ActionType, 0);

  auto pipeline = PipelineDefinition(std::move(*transformer));
  pipeline.run(DataSource(input, sizeof(input) - 1),
               DataSink(dest, sizeof(input) - 1), action);

  EXPECT_EQ("hello world", std::string(dest));
}

}  // namespace metal
