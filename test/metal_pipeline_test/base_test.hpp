#pragma once

#include "gtest/gtest.h"
#include <lmdb.h>

class BaseTest : public ::testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};
