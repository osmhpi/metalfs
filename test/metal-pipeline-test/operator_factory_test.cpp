#include "gtest/gtest.h"

#include <metal-pipeline/operator_factory.hpp>

namespace metal {

TEST(OperatorFactoryTest, DetectsOperatorsInJSON) {
  std::string json =
      "{ \"operators\": { \"changecase\": { \"internal_id\": 1, "
      "\"description\": \"Transform ASCII strings to lower- or uppercase "
      "(default)\", \"options\": { \"lowercase\": { \"short\": \"l\", "
      "\"type\": \"bool\", \"description\": \"Transform to lowercase\", "
      "\"offset\": 256 } } } } }";

  auto factory = OperatorFactory::fromManifestString(json);

  ASSERT_EQ(1u, factory.size());
}

}  // namespace metal
