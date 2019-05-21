#include "base_test.hpp"

extern "C" {
#include <metal_filesystem/metal.h>
#include <metal_storage/storage.h>
}

void BaseTest::SetUp() {
    ASSERT_EQ(MTL_SUCCESS, mtl_storage_initialize());
}

void BaseTest::TearDown() {
    mtl_storage_deinitialize();
}
