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
    virtual uint64_t run(bool last) = 0;

protected:
    std::shared_ptr<PipelineDefinition> _pipeline;
};

class SnapPipelineRunner : public AbstractPipelineRunner {
public:
    SnapPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline, int card)
      : AbstractPipelineRunner(std::move(pipeline)), _initialized(false), _card(card) {}
    uint64_t run(bool last) override;

protected:
    virtual void pre_run(SnapAction &action, bool initialize) { (void)action; (void)initialize; }
    virtual void post_run(SnapAction &action, bool finalize) { (void)action; (void)finalize; }
    void requireReinitialization() { _initialized = false; }

    bool _initialized;
    int _card;
};

class ProfilingPipelineRunner : public SnapPipelineRunner {
public:
    ProfilingPipelineRunner(std::shared_ptr<PipelineDefinition> pipeline, int card)
        : SnapPipelineRunner(std::move(pipeline), card), _results(ProfilingResults{}) {}
    void selectOperatorForProfiling(std::shared_ptr<AbstractOperator> op);
    std::string formatProfilingResults();
    void resetResults() { _results = ProfilingResults{}; };

protected:
    struct ProfilingResults {
        uint64_t global_clock_counter;
        uint64_t input_data_byte_count;
        uint64_t input_transfer_cycle_count;
        uint64_t input_slave_idle_count;
        uint64_t input_master_idle_count;
        uint64_t output_data_byte_count;
        uint64_t output_transfer_cycle_count;
        uint64_t output_slave_idle_count;
        uint64_t output_master_idle_count;
    };

    void pre_run(SnapAction &action, bool initialize) override;
    void post_run(SnapAction &action, bool finalize) override;
    template<typename ... Args>
    static std::string string_format( const std::string& format, Args ... args);

    std::shared_ptr<AbstractOperator> _op;
    ProfilingResults _results;
};

class MockPipelineRunner : public AbstractPipelineRunner {
public:
    uint64_t run(bool last) override;

};

} // namespace metal
