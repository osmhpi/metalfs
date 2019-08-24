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

namespace metal {

static void fill_payload(uint8_t *buffer, uint64_t length) {
    for (uint64_t i = 0; i < length; ++i) {
        buffer[i] = i;
    }
}

TEST(NVMePipelineTest, TransferBlockToNVMe) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    auto dataSource = std::make_shared<HostMemoryDataSource>(src, n_bytes);

    std::vector<mtl_file_extent> file = {{0, 1ul << 20}};  // 1 MB
    auto dataSink = std::make_shared<FileDataSink>(file, 0, n_bytes);

    SnapAction action(fpga::ActionType, 0);

    auto pipeline = PipelineDefinition({ dataSource, dataSink });
    ASSERT_NO_THROW(pipeline.run(action));

    free(src);
}

TEST(NVMePipelineTest, WriteAndReadBlock) {

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    auto *src = reinterpret_cast<uint8_t*>(memalign(4096, n_bytes));
    fill_payload(src, n_bytes);

    std::vector<mtl_file_extent> file = {{0, 1ul << 20}};  // 1 MB

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

    free(src);
    free(dest);
}

} // namespace
