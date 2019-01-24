#include "pipeline_runner.hpp"
#include "abstract_operator.hpp"


namespace metal {

void AbstractOperator::configure(SnapAction &action) {
    if (_profilingEnabled) {

    }
}

void AbstractOperator::finalize(SnapAction &action) {
    if (_profilingEnabled) {
        // TODO
    }
}

} // namespace metal