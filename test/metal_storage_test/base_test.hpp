#pragma once

#include <lmdb.h>
#include "gtest/gtest.h"

class BaseTest : public ::testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};
