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
    SnapPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline, int card)
      : AbstractPipelineRunner(std::move(pipeline)), _initialized(false), _card(card) {}
    void run(bool finalize) override;

protected:
    virtual void initialize(SnapAction &action) { (void)action; }
    virtual void finalize(SnapAction &action) { (void)action; }
    void requireReinitialization() { _initialized = false; }

    bool _initialized;
    int _card;
};

class ProfilingPipelineRunner : public SnapPipelineRunner {
public:
    ProfilingPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline, int card)
        : SnapPipelineRunner(std::move(pipeline), card), _global_clock_counter(0), _performance_counters(10) {}
    void selectOperatorForProfiling(std::shared_ptr<AbstractOperator> op);
    std::string formatProfilingResults();

protected:
    void initialize(SnapAction &action) override;
    void finalize(SnapAction &action) override;
    template<typename ... Args>
    static std::string string_format( const std::string& format, Args ... args);


    std::shared_ptr<AbstractOperator> _op;
    uint64_t _global_clock_counter{};
    std::vector<uint32_t> _performance_counters;

};

class MockPipelineRunner : public AbstractPipelineRunner {
public:
    void run(bool finalize) override;

};

} // namespace metal
