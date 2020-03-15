#pragma once

#include <gtest/gtest.h>
#include <lmdb.h>

#include <metal-filesystem/metal.h>

class BaseTest : public ::testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();

  void test_initialize_env();
  MDB_txn *test_create_txn();
  void test_commit_txn(MDB_txn *txn);

  MDB_env *env;
};

class MetalTest : public BaseTest {
 protected:
  virtual void SetUp();
  virtual void TearDown();

  mtl_context *_context;
};
