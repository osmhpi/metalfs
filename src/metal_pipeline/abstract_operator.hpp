#pragma once

#include <vector>
#include <string>
#include "snap_action.hpp"

namespace metal {

class AbstractOperator {
public:
    virtual void parseArguments(std::vector<std::string> args) = 0;
    virtual void configure(SnapAction &action);
    virtual void finalize(SnapAction &action);

    virtual std::string id() const = 0;
    virtual uint8_t temp_enable_id() const = 0;
    virtual uint8_t temp_stream_id() const = 0;

protected:
    bool _profilingEnabled {false};
};

} // namespace metal
