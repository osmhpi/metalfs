#pragma once

#include <memory>
#include <vector>
#include "abstract_operator.hpp"
#include "data_sink.hpp"
#include "data_source.hpp"

namespace metal {

class PipelineDefinition {
public:
    explicit PipelineDefinition(std::vector<std::shared_ptr<AbstractOperator>> operators)
        : _operators(std::move(operators)), _cached_switch_configuration(false) {
        if (_operators.size() < 2) {
            throw std::runtime_error("Pipeline must contain at least data source and data sink operators.");
        }

        _dataSource = std::dynamic_pointer_cast<DataSource>(_operators[0]);
        if (_dataSource == nullptr) {
            throw std::runtime_error("Pipeline does not start with a data source");
        }

        _dataSink = std::dynamic_pointer_cast<DataSink>(_operators[1]);
        if (_dataSink == nullptr) {
            throw std::runtime_error("Pipeline does not end with a data sink");
        }
    }

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
