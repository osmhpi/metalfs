#include <malloc.h>
#include <metal_pipeline/data_source.hpp>
#include <memory>
#include <metal_pipeline/data_sink.hpp>
#include <metal_fpga/hw/hls/include/snap_action_metal.h>
#include <metal_pipeline/pipeline_definition.hpp>
#include <metal_filesystem_pipeline/file_data_sink.hpp>
#include <metal_filesystem_pipeline/file_data_source.hpp>
#include <gtest/gtest.h>
#include <metal_pipeline/snap_action.hpp>

#include "base_test.hpp"

namespace metal {

using NVMePipelineTest = BaseTest;

TEST_F(NVMePipelineTest, TransfersBlockFromNVMe) {

    uint64_t n_blocks = 1;
    uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    // Read unwritten data
    std::vector<mtl_file_extent> file = {{0, (1ul << 20) / fpga::StorageBlockSize}};  // 1 MB
    auto dataSource = std::make_shared<FileDataSource>(file, 0, n_bytes);

    auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    ASSERT_NO_THROW(pipeline.run(action));

    free(dest);
}

TEST_F(NVMePipelineTest, TransfersBlockToNVMe) {

    uint64_t n_blocks = 1;
    uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);

    std::vector<mtl_file_extent> file = {{0, (1ul << 20) / fpga::StorageBlockSize}};  // 1 MB
    auto dataSink = std::make_shared<FileDataSink>(file, 0, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    ASSERT_NO_THROW(pipeline.run(action));

    free(src);
}

TEST_F(NVMePipelineTest, ReadBlockHasPreviouslyWrittenContents) {

    uint64_t n_blocks = 1;
    uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    std::vector<mtl_file_extent> file = {{0, (1ul << 20) / fpga::StorageBlockSize}};  // 1 MB

    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    SnapAction action(fpga::ActionType, 0);

    {   // Write
        auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);

        auto dataSink = std::make_shared<FileDataSink>(file, 0, n_bytes);

        auto pipeline = PipelineDefinition({dataSource, dataSink});
        ASSERT_NO_THROW(pipeline.run(action));
    }

    {   // Read
        auto dataSource = std::make_shared<FileDataSource>(file, 0, n_bytes);

        auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

        auto pipeline = PipelineDefinition({dataSource, dataSink});
        ASSERT_NO_THROW(pipeline.run(action));
    }

    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

TEST_F(NVMePipelineTest, WritingInMiddleOfFilePreservesSurroundingContents) {

    uint64_t n_blocks = 10;
    uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    std::vector<mtl_file_extent> file = {{0, (1ul << 20) / fpga::StorageBlockSize}};  // 1 MB
    auto *dest = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));

    uint64_t newBytesSize = fpga::StorageBlockSize + 4711;
    uint64_t newBytesOffset = 42;
    auto newBytes = reinterpret_cast<uint8_t*>(malloc(newBytesSize));
    fill_payload(newBytes, newBytesSize);

    SnapAction action(fpga::ActionType, 0);

    {   // Write
        auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);

        auto dataSink = std::make_shared<FileDataSink>(file, 0, n_bytes);

        auto pipeline = PipelineDefinition({dataSource, dataSink});
        ASSERT_NO_THROW(pipeline.run(action));
    }

    {   // Write new bytes
        auto dataSource = std::make_shared<HostMemoryDataSource>(newBytes, newBytesSize);

        auto dataSink = std::make_shared<FileDataSink>(file, newBytesOffset, newBytesSize);

        auto pipeline = PipelineDefinition({dataSource, dataSink});
        ASSERT_NO_THROW(pipeline.run(action));
    }

    {   // Read
        auto dataSource = std::make_shared<FileDataSource>(file, 0, n_bytes);

        auto dataSink = std::make_shared<HostMemoryDataSink>(dest, n_bytes);

        auto pipeline = PipelineDefinition({dataSource, dataSink});
        ASSERT_NO_THROW(pipeline.run(action));
    }

    EXPECT_EQ(0, memcmp(src, dest, newBytesOffset));
    EXPECT_EQ(0, memcmp(newBytes, dest+newBytesOffset, newBytesSize));
    EXPECT_EQ(0, memcmp(src+newBytesOffset+newBytesSize, dest+newBytesOffset+newBytesSize, n_bytes-newBytesSize-newBytesOffset));

    free(src);
    free(dest);
    free(newBytes);
}

} // namespace
