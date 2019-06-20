#pragma once

#include <memory>
#include <vector>
#include "abstract_operator.hpp"

namespace metal {

class PipelineDefinition {
public:
    explicit PipelineDefinition(std::vector<std::shared_ptr<AbstractOperator>> operators)
        : _operators(std::move(operators)) {
        // TODO: Validate
        // operators size must be at least 2
        // first operator must be a DataSource
        // last operator must be a DataSink
    }

    const std::vector<std::shared_ptr<AbstractOperator>> & operators() const { return _operators; }

    uint64_t run(SnapAction &action);

protected:
    std::vector<std::shared_ptr<AbstractOperator>> _operators;

    void configureSwitch(SnapAction &action) const;
};

} // namespace metal
