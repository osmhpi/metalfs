extern "C" {
#include <metal/metal.h>
#include <metal/extent.h>
}

#include "base_test.hpp"

namespace {

TEST_F(BaseTest, AllocatesAnExtent) {
    test_initialize_env();

    uint64_t offset = 4711, length = 4;

    {
        MDB_txn *txn = test_create_txn();
        ASSERT_EQ(MTL_SUCCESS, mtl_initialize_extents(txn, 8));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, NULL, &offset, true));
        EXPECT_EQ(0, offset);
        test_commit_txn(txn);
    }
}

TEST_F(BaseTest, AllocatesTwoExtents) {
    test_initialize_env();

    uint64_t offset = 4711, length = 4;

    {
        MDB_txn *txn = test_create_txn();
        ASSERT_EQ(MTL_SUCCESS, mtl_initialize_extents(txn, 8));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, NULL, &offset, true));
        EXPECT_EQ(0, offset);
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, NULL, &offset, true));
        EXPECT_EQ(4, offset);
        test_commit_txn(txn);
    }
}

TEST_F(BaseTest, FreesAnExtent) {
    test_initialize_env();

    uint64_t offset = 4711, length = 4;

    {
        MDB_txn *txn = test_create_txn();
        ASSERT_EQ(MTL_SUCCESS, mtl_initialize_extents(txn, 8));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, NULL, &offset, true));
        EXPECT_EQ(0, offset);
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(MTL_SUCCESS, mtl_free_extent(txn, offset));
        test_commit_txn(txn);
    }
}

TEST_F(BaseTest, MakesFreedSpaceAvailableForReuse) {
    test_initialize_env();

    uint64_t offset = 4711, length = 4;

    {
        MDB_txn *txn = test_create_txn();
        ASSERT_EQ(MTL_SUCCESS, mtl_initialize_extents(txn, 8));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, NULL, &offset, true));
        EXPECT_EQ(0, offset);
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(MTL_SUCCESS, mtl_free_extent(txn, offset));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(8, mtl_reserve_extent(txn, 8, NULL, &offset, true));
        EXPECT_EQ(0, offset);
        test_commit_txn(txn);
    }
}

} // namespace
