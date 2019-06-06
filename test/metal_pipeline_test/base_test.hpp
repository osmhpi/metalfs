#pragma once

#include <metal_pipeline/operator_registry.hpp>
#include <gtest/gtest.h>

namespace metal {

class PipelineTest : public ::testing::Test {
protected:
    void SetUp() override;

    std::unique_ptr<OperatorRegistry> _registry;
};

// Aliases for selecting tests during simulation
using SimulationPipelineTest = PipelineTest;
using SimulationTest = ::testing::Test;
}