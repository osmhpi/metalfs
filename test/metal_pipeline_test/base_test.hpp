#pragma once

#include <metal_pipeline/operator_registry.hpp>
#include "gtest/gtest.h"

namespace metal {

class BaseTest : public ::testing::Test {
protected:
    virtual void SetUp();

    virtual void TearDown();

    std::unique_ptr<OperatorRegistry> _registry;
};

}