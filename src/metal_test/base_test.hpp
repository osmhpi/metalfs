#pragma once

#include "gtest/gtest.h"

class MetalTest : public ::testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};
