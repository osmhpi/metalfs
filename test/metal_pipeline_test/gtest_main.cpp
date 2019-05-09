#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/stat.h>

#include <spdlog/spdlog.h>
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  // spdlog::set_level(spdlog::level::debug);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
