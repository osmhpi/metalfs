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

static void fill_payload(uint8_t *buffer, uint64_t length) {
    for (uint64_t i = 0; i < length; ++i) {
        buffer[i] = i % 256;
    }
}

TEST_F(SimulationPipelineTest, ColorfilterPipeline_Runs) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    auto filter = _registry->operators().at("colorfilter");

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, filter, dataSink });
    pipeline.run(action);

    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

} // namespace metal
