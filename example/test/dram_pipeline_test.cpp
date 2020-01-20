#include <gtest/gtest.h>
#include <malloc.h>
#include <snap_action_metal.h>
#include <memory>
#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_action.hpp>
#include "base_test.hpp"

namespace metal {

using DRAMPipeline = BaseTest;

TEST_F(DRAMPipeline, TransferBlockToDRAM) {
  uint64_t n_pages = 1;
  uint64_t n_bytes = n_pages * 4096;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  SnapAction action(fpga::ActionType, 0);

  auto pipeline = Pipeline();
  ASSERT_NO_THROW(pipeline.run(
      DataSource(src, n_bytes),
      DataSink(1ul << 31, n_bytes, fpga::AddressType::CardDRAM), action));

  free(src);
}

TEST_F(DRAMPipeline, WriteAndReadBlock) {
  uint64_t n_pages = 1;
  uint64_t n_bytes = n_pages * 4096;
  auto *src = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));
  fill_payload(src, n_bytes);

  auto *dest = reinterpret_cast<uint8_t *>(memalign(4096, n_bytes));

  SnapAction action(fpga::ActionType, 0);

  {  // Write
    auto pipeline = Pipeline();
    ASSERT_NO_THROW(pipeline.run(
        DataSource(src, n_bytes),
        DataSink(1ul << 31, n_bytes, fpga::AddressType::CardDRAM), action));
  }

  {  // Read
    auto pipeline = Pipeline();
    ASSERT_NO_THROW(pipeline.run(
        DataSource(1ul << 31, n_bytes, fpga::AddressType::CardDRAM),
        DataSink(src, n_bytes), action));
  }

  free(src);
  free(dest);
}

}  // namespace metal
