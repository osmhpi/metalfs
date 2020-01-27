#include "base_test.hpp"

#include <metal-pipeline/operator_factory.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

using ImageInfo = SimulationTest;

TEST_F(ImageInfo, ReadsImageInfo) {
  SnapAction action;
  ASSERT_NO_THROW({
    auto factory = OperatorFactory::fromFPGA(action);
    ASSERT_GT(factory.operatorSpecifications().size(), 0);
  });
}

}  // namespace metal
