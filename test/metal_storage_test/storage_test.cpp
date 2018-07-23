#include <malloc.h>

#include <metal/metal.h>
#include <metal_storage/storage.h>

#include "base_test.hpp"

namespace {

TEST_F(BaseTest, Storage_WriteData) {

    mtl_file_extent extents[] = {
        { .offset=0 , .length=1 } // in blocks
    };
    ASSERT_EQ(MTL_SUCCESS, mtl_storage_set_active_write_extent_list(extents, sizeof(extents) / sizeof(extents[0])));

    uint64_t n_bytes = 128;
    void *buffer = calloc(n_bytes, 1);

    EXPECT_EQ(MTL_SUCCESS, mtl_storage_write(0, buffer, n_bytes));

    free(buffer);
}

TEST_F(BaseTest, Storage_WriteAndReadData) {

    mtl_file_extent extents[] = {
        { .offset=0 , .length=1 } // in blocks
    };

    uint64_t n_pages = 1;
    uint64_t n_bytes = n_pages * 4096;
    void *src = memalign(4096, n_bytes);
    memset(src, 1, n_bytes);
    void *dest = memalign(4096, n_bytes);

    ASSERT_EQ(MTL_SUCCESS, mtl_storage_set_active_write_extent_list(extents, sizeof(extents) / sizeof(extents[0])));
    EXPECT_EQ(MTL_SUCCESS, mtl_storage_write(0, src, n_bytes));

    ASSERT_EQ(MTL_SUCCESS, mtl_storage_set_active_read_extent_list(extents, sizeof(extents) / sizeof(extents[0])));
    EXPECT_EQ(MTL_SUCCESS, mtl_storage_read(0, dest, n_bytes));

    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

// static void print_memory_64(void * mem)
// {
//     for (int i = 0; i < 64; ++i) {
//         printf("%02x ", ((uint8_t*)mem)[i]);
//         if (i%8 == 7) printf("\n");
//     }
// }

TEST_F(BaseTest, Storage_WriteAndReadDataUsingMultipleExtents) {

    mtl_file_extent extents[] = {
        { .offset=0   , .length=1 }, // in blocks
        { .offset=100 , .length=1 }
    };

    uint64_t n_pages = 2;
    uint64_t n_bytes = n_pages * 4096;
    void *src = memalign(4096, n_bytes);
    memset(src, 1, n_bytes);
    void *dest = memalign(4096, n_bytes);

    ASSERT_EQ(MTL_SUCCESS, mtl_storage_set_active_write_extent_list(extents, sizeof(extents) / sizeof(extents[0])));
    EXPECT_EQ(MTL_SUCCESS, mtl_storage_write(0, src, n_bytes));
    
    ASSERT_EQ(MTL_SUCCESS, mtl_storage_set_active_read_extent_list(extents, sizeof(extents) / sizeof(extents[0])));
    EXPECT_EQ(MTL_SUCCESS, mtl_storage_read(0, dest, n_bytes));

    EXPECT_EQ(0, memcmp(src, dest, n_bytes));

    free(src);
    free(dest);
}

} // namespace
