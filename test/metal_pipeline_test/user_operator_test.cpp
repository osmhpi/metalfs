#include "gtest/gtest.h"

#include <metal_pipeline/operator_registry.hpp>

namespace metal {

TEST(UserOperatorTest, ParsesOperatorID) {

    UserOperator op("./test/metal_pipeline_test/operators/blowfish_encrypt.json");

    ASSERT_EQ(op.id(), "blowfish_encrypt");
}


TEST(UserOperatorTest, AllowsToSetOptions) {

    UserOperator op("./test/metal_pipeline_test/operators/blowfish_encrypt.json");

    ASSERT_NO_THROW(op.setOption("key", std::make_unique<std::vector<char>>(128)));
}

TEST(UserOperatorTest, RevokesInvalidOptions) {

    UserOperator op("./test/metal_pipeline_test/operators/blowfish_encrypt.json");

    ASSERT_ANY_THROW(op.setOption("does_not_exist", false));
}


TEST(UserOperatorTest, RevokesInvalidOptionValues) {

    UserOperator op("./test/metal_pipeline_test/operators/blowfish_encrypt.json");

    ASSERT_ANY_THROW(op.setOption("key", false));
}


} // namespace
