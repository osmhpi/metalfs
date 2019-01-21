#include "base_test.hpp"

extern "C" {
#include <metal/metal.h>
#include <metal_pipeline/pipeline.h>
}

void BaseTest::SetUp() {
    ASSERT_EQ(MTL_SUCCESS, mtl_pipeline_initialize());
}

void BaseTest::TearDown() {
    mtl_pipeline_deinitialize();
}
