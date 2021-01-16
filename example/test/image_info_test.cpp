#include "base_test.hpp"

#include <metal-pipeline/operator_factory.hpp>

namespace metal {

using ImageInfo = SimulationTest;

TEST_F(ImageInfo, ReadsImageInfo) {
  auto action = _actionFactory->createAction();

  auto createFactory = [&]() {
    auto factory = OperatorFactory::fromFPGA(*action);
    ASSERT_GT(factory.operatorSpecifications().size(), 0);
  };

  ASSERT_NO_THROW(createFactory());
}

}  // namespace metal
