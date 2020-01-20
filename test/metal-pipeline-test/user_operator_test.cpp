#include "gtest/gtest.h"

#include <metal-pipeline/operator_registry.hpp>
#include <metal-pipeline/operator_specification.hpp>

namespace metal {

const char* OperatorJson =
    R"({"id":"blowfish_encrypt","description":"Encrypt data with the blowfish cipher","prepare_required":true,"options":{"key":{"short":"k","type":{"type":"buffer","size":16},"description":"The encryption key to use","offset":256}},"internal_id":4})";

TEST(UserOperatorTest, UserOperatorSpec_Parses_Specification) {
  OperatorSpecification spec("blowfish_encrypt", OperatorJson);

  ASSERT_EQ(spec.id(), "blowfish_encrypt");
  ASSERT_EQ(spec.streamID(), 4);
  ASSERT_EQ(spec.description(), "Encrypt data with the blowfish cipher");
  ASSERT_EQ(spec.prepare_required(), true);
}

TEST(UserOperatorTest, UserOperator_AllowsToSetOptions) {
  auto spec = std::make_shared<OperatorSpecification>("blowfish_encrypt",
                                                          OperatorJson);
  Operator op(spec);

  ASSERT_NO_THROW(
      op.setOption("key", std::make_unique<std::vector<char>>(128)));
}

TEST(UserOperatorTest, UserOperator_RevokesInvalidOptions) {
  auto spec = std::make_shared<OperatorSpecification>("blowfish_encrypt",
                                                          OperatorJson);
  Operator op(spec);

  ASSERT_ANY_THROW(op.setOption("does_not_exist", false));
}

TEST(UserOperatorTest, UserOperator_RevokesInvalidOptionValues) {
  auto spec = std::make_shared<OperatorSpecification>("blowfish_encrypt",
                                                          OperatorJson);
  Operator op(spec);

  ASSERT_ANY_THROW(op.setOption("key", false));
}

}  // namespace metal
