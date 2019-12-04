#include <malloc.h>
#include <metal_pipeline/data_source.hpp>
#include <memory>
#include <metal_pipeline/data_sink.hpp>
#include <metal_fpga/hw/hls/include/snap_action_metal.h>
#include <metal_pipeline/pipeline_definition.hpp>
#include <gtest/gtest.h>
#include <metal_pipeline/pipeline_runner.hpp>
#include <metal_pipeline/snap_action.hpp>
#include "base_test.hpp"

namespace metal {

using ReadWritePipeline = SimulationTest;

TEST_F(ReadWritePipeline, TransfersSmallBuffer) {

    uint64_t n_bytes = 42;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    uint64_t size;
    ASSERT_NO_THROW(size = pipeline.run(action));

    EXPECT_EQ(n_bytes, size);
    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

TEST_F(ReadWritePipeline, ToleratesTooLargeOutputBuffer) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * fpga::StorageBlockSize;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, 2*fpga::StorageBlockSize));

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, 2*fpga::StorageBlockSize);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    uint64_t size;
    ASSERT_NO_THROW(size = pipeline.run(action));

    EXPECT_EQ(n_bytes, size);
    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

TEST_F(ReadWritePipeline, TransfersEntirePage) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    uint64_t size;
    ASSERT_NO_THROW(size = pipeline.run(action));

    EXPECT_EQ(n_bytes, size);
    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

TEST_F(ReadWritePipeline, TransfersEntirePageToInternalSink) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);
    auto dataSink = std::make_shared<NullDataSink>(n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    ASSERT_NO_THROW(pipeline.run(action));

    free(src);
}

TEST_F(ReadWritePipeline, TransfersEntirePageFromInternalDataGenerator) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;

    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    auto dataSource = std::make_shared<RandomDataSource>(n_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    uint64_t size;
    ASSERT_NO_THROW(size = pipeline.run(action));

    EXPECT_EQ(n_bytes, size);

    free(dest);
}

TEST_F(ReadWritePipeline, TransfersEntirePageFromInternalDataGeneratorToInternalSink) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;

    auto dataSource = std::make_shared<RandomDataSource>(n_bytes);
    auto dataSink = std::make_shared<NullDataSink>(n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    ASSERT_NO_THROW(pipeline.run(action));
}

TEST_F(ReadWritePipeline, TransfersUnalignedDataSpanningMultiplePages) {

    uint64_t src_pages = 3;
    uint64_t src_bytes = src_pages * 4096;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, src_bytes));

    uint64_t src_offset = 1815;
    uint64_t dest_offset = 2;
    uint64_t payload_bytes = 4711;

    fill_payload(src + src_offset, payload_bytes);

    uint64_t dest_pages = 2;
    uint64_t dest_bytes = dest_pages * 4096;
    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, dest_bytes));

    auto dataSource = std::make_shared<HostMemoryDataSource>(src + src_offset, payload_bytes);
    auto dataSink = std::make_shared<HostMemoryDataSink>(dest + dest_offset, payload_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    uint64_t size;
    ASSERT_NO_THROW(size = pipeline.run(action));

    EXPECT_EQ(payload_bytes, size);
    EXPECT_EQ(0, memcmp(src + src_offset, dest + dest_offset, payload_bytes));

    free(src);
    free(dest);
}

} // namespace
