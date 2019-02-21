#pragma once

#include <metal_pipeline/pipeline_runner.hpp>

namespace metal {

class FilesystemPipelineRunner : public ProfilingPipelineRunner {
public:
    explicit FilesystemPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline) : ProfilingPipelineRunner(std::move(pipeline)) {}

protected:
    void initialize(SnapAction &action) override;
};

} // namespace metal
