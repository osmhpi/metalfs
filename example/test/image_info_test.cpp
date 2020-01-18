#include "base_test.hpp"

#include <metal-pipeline/snap_pipeline_runner.hpp>

namespace metal {

using ImageInfo = SimulationTest;

TEST_F(ImageInfo, ReadsImageInfo) {
  std::string info;
  ASSERT_NO_THROW(info = SnapPipelineRunner::readImageInfo(0));
  ASSERT_GT(info.size(), 0);
}

}  // namespace metal
