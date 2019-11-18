#pragma once

#include <metal_pipeline/operator_registry.hpp>
#include <gtest/gtest.h>

namespace metal {

class BaseTest : public ::testing::Test {
public:
    static void fill_payload(uint8_t *buffer, uint64_t length);
    static void print_memory_64(void * mem);
    void SetUp() override;
    virtual void _setUp(){};
};

class PipelineTest : public BaseTest {
protected:
    void _setUp() override;
    std::shared_ptr<AbstractOperator> try_get_operator(const std::string &key);

    std::unique_ptr<OperatorRegistry> _registry;
};


class SimulationTest : public BaseTest {
protected:
    void SetUp() override;
};

class SimulationPipelineTest : public PipelineTest {
protected:
    void SetUp() override;
};

}
