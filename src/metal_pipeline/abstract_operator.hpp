#pragma once

namespace metal {

class AbstractOperator {
public:
    virtual void parseArguments() = 0;
    virtual void configure() = 0;
    virtual void finalize() = 0;

    virtual std::string id() const = 0;

protected:
    bool _profilingEnabled {false};
};

} // namespace metal
