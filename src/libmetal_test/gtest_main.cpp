#include "gtest/gtest.h"

#include "../libmetal/metal.h"

namespace {

TEST(MetalTest, Initializes) {
  mtl_initialize();
  EXPECT_EQ(1, 1);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
