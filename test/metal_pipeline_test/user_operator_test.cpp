#include "gtest/gtest.h"

#include <metal_pipeline/operator_registry.hpp>

namespace metal {

const char* OperatorJson = R"({"id":"blowfish_encrypt","description":"Encrypt data with the blowfish cipher","prepare_required":true,"options":{"key":{"short":"k","type":{"type":"buffer","size":16},"description":"The encryption key to use","offset":256}},"internal_id":4})";

 TEST(UserOperatorTest, AssignsOperatorID) {

     UserOperator op("blowfish_encrypt", OperatorJson);

     ASSERT_EQ(op.id(), "blowfish_encrypt");
 }


 TEST(UserOperatorTest, AllowsToSetOptions) {

    UserOperator op("blowfish_encrypt", OperatorJson);

     ASSERT_NO_THROW(op.setOption("key", std::make_unique<std::vector<char>>(128)));
 }

 TEST(UserOperatorTest, RevokesInvalidOptions) {

    UserOperator op("blowfish_encrypt", OperatorJson);

     ASSERT_ANY_THROW(op.setOption("does_not_exist", false));
 }


 TEST(UserOperatorTest, RevokesInvalidOptionValues) {

    UserOperator op("blowfish_encrypt", OperatorJson);

     ASSERT_ANY_THROW(op.setOption("key", false));
 }

} // namespace
