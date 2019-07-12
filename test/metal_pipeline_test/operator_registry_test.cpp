#include "gtest/gtest.h"

#include <metal_pipeline/operator_registry.hpp>

namespace metal {

TEST(OperatorRegistryTest, DetectsOperatorsInJSON) {

    std::string json = "{ \"operators\": { \"changecase\": { \"internal_id\": 1, \"description\": \"Transform ASCII strings to lower- or uppercase (default)\", \"options\": { \"lowercase\": { \"short\": \"l\", \"type\": \"bool\", \"description\": \"Transform to lowercase\", \"offset\": 256 } } } } }";

    OperatorRegistry registry(json);

    ASSERT_EQ(1u, registry.operators().size());
}

} // namespace
