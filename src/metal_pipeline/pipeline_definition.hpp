#pragma once

#include <memory>
#include <vector>
#include "abstract_operator.hpp"
#include "data_sink.hpp"
#include "data_source.hpp"

namespace metal {

class PipelineDefinition {
public:
    explicit PipelineDefinition(std::vector<std::shared_ptr<AbstractOperator>> operators);

    const std::vector<std::shared_ptr<AbstractOperator>> & operators() const { return _operators; }

    uint64_t run(SnapAction &action);
    void configureSwitch(SnapAction &action, bool set_cached);

protected:
    std::vector<std::shared_ptr<AbstractOperator>> _operators;
    std::shared_ptr<DataSource> _dataSource;
    std::shared_ptr<DataSink> _dataSink;
    bool _cached_switch_configuration;

};

} // namespace metal
