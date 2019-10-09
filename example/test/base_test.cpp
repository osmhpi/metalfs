#include "base_test.hpp"

#include <libgen.h>
#include <metal_pipeline/pipeline_runner.hpp>

namespace metal {

void BaseTest::fill_payload(uint8_t *buffer, uint64_t length) {
    for (uint64_t i = 0; i < length; ++i) {
        buffer[i] = i % 256;
    }
}


void PipelineTest::SetUp() {
    auto info = SnapPipelineRunner::readImageInfo(0);
    _registry = std::make_unique<OperatorRegistry>(info);
}

std::shared_ptr<AbstractOperator> PipelineTest::try_get_operator(const std::string &key) {
    auto op = _registry->operators().find(key);
    if (op == _registry->operators().end()) {
        return nullptr;
    }
    return op->second;
}

void SimulationPipelineTest::SetUp() {  
#ifndef __PPC64__
    GTEST_SKIP();
#endif
    PipelineTest::SetUp();
}

void SimulationTest::SetUp() {  
#ifndef __PPC64__
    GTEST_SKIP();
#endif
    BaseTest::SetUp();
}
} // namespace metal
