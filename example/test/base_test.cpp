#include "base_test.hpp"

#include <libgen.h>
#include <metal_pipeline/pipeline_runner.hpp>

namespace metal {

void PipelineTest::SetUp() {

    auto info = SnapPipelineRunner::readImageInfo(0);
    _registry = std::make_unique<OperatorRegistry>(info);
}

} // namespace metal
