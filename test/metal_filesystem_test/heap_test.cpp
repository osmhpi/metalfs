extern "C" {
#include <metal/metal.h>
#include <metal/heap.h>
}

#include "base_test.hpp"

namespace {

TEST_F(BaseTest, Heap_IsEmptyByDefault) {
    test_initialize_env();

    MDB_txn *txn = test_create_txn();
    uint64_t max_value;
    EXPECT_EQ(MTL_ERROR_NOENTRY, mtl_heap_extract_max(txn, &max_value));

    test_commit_txn(txn);
}

TEST_F(BaseTest, Heap_Returns_Inserted_Element) {
    test_initialize_env();

    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 4711;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t max_value;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_extract_max(txn, &max_value));
        test_commit_txn(txn);

        EXPECT_EQ(4711, max_value);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t max_value;
        EXPECT_EQ(MTL_ERROR_NOENTRY, mtl_heap_extract_max(txn, &max_value));
        test_commit_txn(txn);
    }
}

TEST_F(BaseTest, Heap_Returns_Inserted_Elements_In_Priority_Order) {
    test_initialize_env();

    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 4711;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 4712;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t max_value;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_extract_max(txn, &max_value));
        test_commit_txn(txn);

        EXPECT_EQ(4712, max_value);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t max_value;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_extract_max(txn, &max_value));
        test_commit_txn(txn);

        EXPECT_EQ(4711, max_value);
    }
}

TEST_F(BaseTest, Heap_Correctly_Processes_Complex_Operation_Sequence) {
    test_initialize_env();

    // Remember the node ids of a few inserted elements
    mtl_heap_node_id node_45_id, node_47_id, node_42_id;

    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 24;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 22;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 29;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 49;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 3;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 42;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, &node_42_id));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 25;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 45;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, &node_45_id));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 40;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 37;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 34;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 32;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t max_value;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_extract_max(txn, &max_value));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t value = 47;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_insert(txn, value, value, &node_47_id));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_increase_key(txn, node_45_id, 49, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_increase_key(txn, node_47_id, 48, NULL));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t max_value;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_extract_max(txn, &max_value));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        uint64_t max_value;
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_extract_max(txn, &max_value));
        test_commit_txn(txn);
    }
    {
        MDB_txn *txn = test_create_txn();
        EXPECT_EQ(MTL_SUCCESS, mtl_heap_increase_key(txn, node_42_id, 44, NULL));
        test_commit_txn(txn);
    }
}

} // namespace
