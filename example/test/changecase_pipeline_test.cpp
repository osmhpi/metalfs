#include <malloc.h>

#include <gtest/gtest.h>

#include <metal_fpga/hw/hls/include/snap_action_metal.h>
#include <metal_pipeline/operator_registry.hpp>
#include <metal_pipeline/pipeline_definition.hpp>
#include <metal_pipeline/data_sink.hpp>
#include <metal_pipeline/data_source.hpp>
#include <metal_pipeline/snap_action.hpp>

#include "base_test.hpp"

namespace metal {

TEST_F(SimulationPipelineTest, ChangecasePipeline_TransformsToUppercase) {

    const char input[] = "Hello World";
    char dest[20] = { 0 };

    auto transformer = _registry->operators().at("changecase");
    transformer->setOption("lowercase", false);

    auto dataSource = std::make_shared<HostMemoryDataSource>(input, sizeof(input) - 1);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, sizeof(input) - 1);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, transformer, dataSink });
    pipeline.run(action);

    EXPECT_EQ("HELLO WORLD", std::string(dest));
}

TEST_F(SimulationPipelineTest, ChangecasePipeline_TransformsToLowercase) {

    const char input[] = "Hello World";
    char dest[20] = { 0 };

    auto transformer = _registry->operators().at("changecase");
    transformer->setOption("lowercase", true);

    auto dataSource = std::make_shared<HostMemoryDataSource>(input, sizeof(input) - 1);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, sizeof(input) - 1);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, transformer, dataSink });
    pipeline.run(action);

    EXPECT_EQ("hello world", std::string(dest));
}

} // namespace metal
