#pragma once

#include <memory>
#include <vector>
#include "abstract_operator.hpp"

namespace metal {

class PipelineDefinition {
public:
    explicit PipelineDefinition(std::vector<std::shared_ptr<AbstractOperator>> operators)
        : _operators(std::move(operators)), _cached_switch_configuration(false) {
        // TODO: Validate
        // operators size must be at least 2
        // first operator must be a DataSource
        // last operator must be a DataSink
    }

    const std::vector<std::shared_ptr<AbstractOperator>> & operators() const { return _operators; }

    uint64_t run(SnapAction &action);
    void configureSwitch(SnapAction &action, bool set_cached);

protected:
    std::vector<std::shared_ptr<AbstractOperator>> _operators;
    bool _cached_switch_configuration;

};

} // namespace metal
