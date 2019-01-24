#include "gtest/gtest.h"

#include <metal_pipeline/operator_registry.hpp>

namespace metal {

TEST(OperatorRegistryTest, DetectsOperatorsInSearchPath) {

    OperatorRegistry registry("./test/metal_pipeline_test/operators");

    ASSERT_EQ(registry.operators().size(), 3);
}

} // namespace
