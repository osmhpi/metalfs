#include <metal/metal.h>
#include <metal/extent.h>

#include "base_test.hpp"

namespace {

TEST_F(MetalTest, AllocatesAnExtent) {
    uint64_t offset = 4711, length = 4;

    {
        MDB_txn *txn = mtl_create_txn();
        ASSERT_EQ(MTL_SUCCESS, mtl_initialize_extents(txn, 8));
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, &offset));
        EXPECT_EQ(0, offset);
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(MTL_SUCCESS, mtl_commit_extent(txn, offset, length));
        mtl_commit_txn(txn);
    }
}

TEST_F(MetalTest, AllocatesTwoExtents) {
    uint64_t offset = 4711, length = 4;

    {
        MDB_txn *txn = mtl_create_txn();
        ASSERT_EQ(MTL_SUCCESS, mtl_initialize_extents(txn, 8));
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, &offset));
        EXPECT_EQ(0, offset);
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, &offset));
        EXPECT_EQ(4, offset);
        mtl_commit_txn(txn);
    }
}

TEST_F(MetalTest, FreesAnExtent) {
    uint64_t offset = 4711, length = 4;

    {
        MDB_txn *txn = mtl_create_txn();
        ASSERT_EQ(MTL_SUCCESS, mtl_initialize_extents(txn, 8));
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, &offset));
        EXPECT_EQ(0, offset);
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(MTL_SUCCESS, mtl_free_extent(txn, offset));
        mtl_commit_txn(txn);
    }
}

TEST_F(MetalTest, MakesFreedSpaceAvailableForReuse) {
    uint64_t offset = 4711, length = 4;

    {
        MDB_txn *txn = mtl_create_txn();
        ASSERT_EQ(MTL_SUCCESS, mtl_initialize_extents(txn, 8));
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(4, mtl_reserve_extent(txn, length, &offset));
        EXPECT_EQ(0, offset);
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(MTL_SUCCESS, mtl_free_extent(txn, offset));
        mtl_commit_txn(txn);
    }
    {
        MDB_txn *txn = mtl_create_txn();
        EXPECT_EQ(8, mtl_reserve_extent(txn, 8, &offset));
        EXPECT_EQ(0, offset);
        mtl_commit_txn(txn);
    }
}

} // namespace
