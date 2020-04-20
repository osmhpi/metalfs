#include <malloc.h>

#include <memory>

#include <gtest/gtest.h>

#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/fpga_interface.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_action.hpp>
#include <metal-pipeline/snap_pipeline_runner.hpp>

#include "base_test.hpp"

namespace metal {

using NVMePipelineTest = BaseTest;

TEST_F(NVMePipelineTest, TransfersBlockFromNVMe) {
  uint64_t n_blocks = 1;
  uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  auto filesystem = std::make_shared<PipelineStorage>(
      Card{0, 10}, fpga::AddressType::NVMe, fpga::MapType::NVMe,
      "./test_metadata", true);
  uint64_t file;
  mtl_create(filesystem->context(), "/test", 0755, &file);
  mtl_truncate(filesystem->context(), file, 1ul << 20);  // 1 MB

  FileDataSourceContext dataSource(filesystem, file, 0, n_bytes);

  DefaultDataSinkContext dataSink(DataSink(dest, n_bytes));

  SnapPipelineRunner runner(Card{0, 10});
  ASSERT_NO_THROW(runner.run(dataSource, dataSink));

  free(dest);
}

TEST_F(NVMePipelineTest, TransfersBlockToNVMe) {
  uint64_t n_blocks = 1;
  uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  auto filesystem = std::make_shared<PipelineStorage>(
      Card{0, 10}, fpga::AddressType::NVMe, fpga::MapType::NVMe,
      "./test_metadata", true);
  uint64_t file;
  mtl_create(filesystem->context(), "/test", 0755, &file);
  mtl_truncate(filesystem->context(), file, 1ul << 20);  // 1 MB

  DefaultDataSourceContext dataSource(DataSource(src, n_bytes));
  FileDataSinkContext dataSink(filesystem, file, 0, n_bytes);

  SnapPipelineRunner runner(Card{0, 10});
  ASSERT_NO_THROW(runner.run(dataSource, dataSink));

  free(src);
}

TEST_F(NVMePipelineTest, ReadBlockHasPreviouslyWrittenContents) {
  uint64_t n_blocks = 1;
  uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  auto filesystem = std::make_shared<PipelineStorage>(
      Card{0, 10}, fpga::AddressType::NVMe, fpga::MapType::NVMe,
      "./test_metadata", true);
  uint64_t file;
  mtl_create(filesystem->context(), "/test", 0755, &file);
  mtl_truncate(filesystem->context(), file, 1ul << 20);  // 1 MB

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  {  // Write
    DefaultDataSourceContext dataSource(DataSource(src, n_bytes));
    FileDataSinkContext dataSink(filesystem, file, 0, n_bytes);

    SnapPipelineRunner runner(Card{0, 10});
    ASSERT_NO_THROW(runner.run(dataSource, dataSink));
  }

  {  // Read
    FileDataSourceContext dataSource(filesystem, file, 0, n_bytes);
    DefaultDataSinkContext dataSink(DataSink(dest, n_bytes));

    SnapPipelineRunner runner(Card{0, 10});
    ASSERT_NO_THROW(runner.run(dataSource, dataSink));
  }

  EXPECT_EQ(0, memcmp(src, dest, n_bytes));

  free(src);
  free(dest);
}

TEST_F(NVMePipelineTest, WritingInMiddleOfFilePreservesSurroundingContents) {
  uint64_t n_blocks = 10;
  uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  auto filesystem = std::make_shared<PipelineStorage>(
      Card{0, 10}, fpga::AddressType::NVMe, fpga::MapType::NVMe,
      "./test_metadata", true);
  uint64_t file;
  mtl_create(filesystem->context(), "/test", 0755, &file);
  mtl_truncate(filesystem->context(), file, 1ul << 20);  // 1 MB

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  uint64_t newBytesSize = fpga::StorageBlockSize + 4711;
  uint64_t newBytesOffset = 42;
  auto newBytes = reinterpret_cast<uint8_t *>(memalign(4096, newBytesSize));
  fill_payload(newBytes, newBytesSize);

  {  // Write
    DefaultDataSourceContext dataSource(DataSource(src, n_bytes));
    FileDataSinkContext dataSink(filesystem, file, 0, n_bytes);

    SnapPipelineRunner runner(Card{0, 10});
    ASSERT_NO_THROW(runner.run(dataSource, dataSink));
  }

  {  // Write new bytes
    DefaultDataSourceContext dataSource(DataSource(newBytes, newBytesSize));
    FileDataSinkContext dataSink(filesystem, file, newBytesOffset,
                                 newBytesSize);

    SnapPipelineRunner runner(Card{0, 10});
    ASSERT_NO_THROW(runner.run(dataSource, dataSink));
  }

  {  // Read
    FileDataSourceContext dataSource(filesystem, file, 0, n_bytes);
    DefaultDataSinkContext dataSink(DataSink(dest, n_bytes));

    SnapPipelineRunner runner(Card{0, 10});
    ASSERT_NO_THROW(runner.run(dataSource, dataSink));
  }

  EXPECT_EQ(0, memcmp(src, dest, newBytesOffset));
  EXPECT_EQ(0, memcmp(newBytes, dest + newBytesOffset, newBytesSize));
  EXPECT_EQ(0, memcmp(src + newBytesOffset + newBytesSize,
                      dest + newBytesOffset + newBytesSize,
                      n_bytes - newBytesSize - newBytesOffset));

  free(src);
  free(dest);
  free(newBytes);
}

}  // namespace metal
