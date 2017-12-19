#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/stat.h>

#include "gtest/gtest.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
