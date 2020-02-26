#include <gtest/gtest.h>
#include <malloc.h>
#include <metal-pipeline/fpga_interface.hpp>
#include <memory>
#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
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

  // Read unwritten data
  std::vector<mtl_file_extent> file = {
      {0, (1ul << 20) / fpga::StorageBlockSize}};  // 1 MB
  FileDataSourceContext dataSource(fpga::AddressType::NVMe,
                                      fpga::MapType::NVMe, file, 0, n_bytes);

  DefaultDataSinkContext dataSink(DataSink(dest, n_bytes));

  SnapPipelineRunner runner(0);
  ASSERT_NO_THROW(runner.run(dataSource, dataSink));

  free(dest);
}

TEST_F(NVMePipelineTest, TransfersBlockToNVMe) {
  uint64_t n_blocks = 1;
  uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  DefaultDataSourceContext dataSource(DataSource(src, n_bytes));

  std::vector<mtl_file_extent> file = {
      {0, (1ul << 20) / fpga::StorageBlockSize}};  // 1 MB
  FileDataSinkContext dataSink(fpga::AddressType::NVMe, fpga::MapType::NVMe,
                                  file, 0, n_bytes);

  SnapPipelineRunner runner(0);
  ASSERT_NO_THROW(runner.run(dataSource, dataSink));

  free(src);
}

TEST_F(NVMePipelineTest, ReadBlockHasPreviouslyWrittenContents) {
  uint64_t n_blocks = 1;
  uint64_t n_bytes = n_blocks * fpga::StorageBlockSize;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  std::vector<mtl_file_extent> file = {
      {0, (1ul << 20) / fpga::StorageBlockSize}};  // 1 MB

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  {  // Write
    DefaultDataSourceContext dataSource(DataSource(src, n_bytes));
    FileDataSinkContext dataSink(fpga::AddressType::NVMe,
                                    fpga::MapType::NVMe, file, 0, n_bytes);

    SnapPipelineRunner runner(0);
    ASSERT_NO_THROW(runner.run(dataSource, dataSink));
  }

  {  // Read
    FileDataSourceContext dataSource(fpga::AddressType::NVMe,
                                        fpga::MapType::NVMe, file, 0, n_bytes);
    DefaultDataSinkContext dataSink(DataSink(dest, n_bytes));

    SnapPipelineRunner runner(0);
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

  std::vector<mtl_file_extent> file = {
      {0, (1ul << 20) / fpga::StorageBlockSize}};  // 1 MB
  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  uint64_t newBytesSize = fpga::StorageBlockSize + 4711;
  uint64_t newBytesOffset = 42;
  auto newBytes = reinterpret_cast<uint8_t *>(memalign(4096, newBytesSize));
  fill_payload(newBytes, newBytesSize);

  {  // Write
    DefaultDataSourceContext dataSource(DataSource(src, n_bytes));
    FileDataSinkContext dataSink(fpga::AddressType::NVMe,
                                    fpga::MapType::NVMe, file, 0, n_bytes);

    SnapPipelineRunner runner(0);
    ASSERT_NO_THROW(runner.run(dataSource, dataSink));
  }

  {  // Write new bytes
    DefaultDataSourceContext dataSource(
        DataSource(newBytes, newBytesSize));
    FileDataSinkContext dataSink(fpga::AddressType::NVMe,
                                    fpga::MapType::NVMe, file, newBytesOffset,
                                    newBytesSize);

    SnapPipelineRunner runner(0);
    ASSERT_NO_THROW(runner.run(dataSource, dataSink));
  }

  {  // Read
    FileDataSourceContext dataSource(fpga::AddressType::NVMe,
                                        fpga::MapType::NVMe, file, 0, n_bytes);
    DefaultDataSinkContext dataSink(DataSink(dest, n_bytes));

    SnapPipelineRunner runner(0);
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
