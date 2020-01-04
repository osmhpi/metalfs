#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <memory>
#include <vector>
#include <metal-pipeline/operator_runtime_context.hpp>
#include <metal-pipeline/data_sink.hpp>
#include <metal-pipeline/data_source.hpp>

namespace metal {

class DataSource;
class DataSink;
class UserOperator;

class METAL_PIPELINE_API PipelineDefinition {
public:
    explicit PipelineDefinition(std::vector<UserOperator> userOperators);
    template<typename... Ts>
    PipelineDefinition(Ts... userOperators) : PipelineDefinition({ userOperators... }) {}
    explicit PipelineDefinition(std::vector<UserOperatorRuntimeContext> userOperators);

    std::vector<UserOperatorRuntimeContext> & operators() { return _operators; }

    uint64_t run(DataSource dataSource, DataSink dataSink, SnapAction &action);
    uint64_t run(DataSourceRuntimeContext &dataSource, DataSinkRuntimeContext &dataSink, SnapAction &action);

    void configureSwitch(SnapAction &action, bool set_cached);

protected:
    std::vector<UserOperatorRuntimeContext> _operators;
    bool _cached_switch_configuration;

};

} // namespace metal
