#include "base_test.hpp"

#include <metal/metal.h>
#include <metal_storage/storage.h>

void BaseTest::SetUp() {
    mtl_storage_initialize();
}

void BaseTest::TearDown() {
    mtl_storage_deinitialize();
}
