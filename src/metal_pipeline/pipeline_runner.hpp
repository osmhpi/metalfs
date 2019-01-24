#include <utility>

#pragma once

#include <memory>
#include <vector>
#include "pipeline_definition.hpp"

namespace metal {

// Runs a pipeline one or more times
class AbstractPipelineRunner {
public:
    explicit AbstractPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline) : _pipeline(pipeline) {}
    virtual void run(bool finalize) = 0;

protected:
    std::shared_ptr<PipelineDefinition> _pipeline;
};

class SnapPipelineRunner : public AbstractPipelineRunner {
public:
    explicit SnapPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline) : AbstractPipelineRunner(std::move(pipeline)) {}
    void run(bool finalize) override;

protected:
    virtual void initialize(SnapAction &action) {}
    virtual void finalize(SnapAction &action) {}
    void requireReinitiailzation() { _initialized = false; }

    bool _initialized = false;
};

class ProfilingPipelineRunner : public SnapPipelineRunner {
public:
    explicit ProfilingPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline)
        : SnapPipelineRunner(std::move(pipeline)), _performance_counters(10), _global_clock_counter(0) {}
    void selectOperatorForProfiling(std::shared_ptr<AbstractOperator> op);
    void printProfilingResults();

protected:
    void initialize(SnapAction &action) override;
    void finalize(SnapAction &action) override;

protected:
    std::shared_ptr<AbstractOperator> _op;
    uint64_t _global_clock_counter{};
    std::vector<uint32_t> _performance_counters;

};

class MockPipelineRunner : public AbstractPipelineRunner {
public:
    void run(bool finalize) override;

};

} // namespace metal
