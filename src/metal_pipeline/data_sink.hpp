#pragma once

#include <string>
#include "abstract_operator.hpp"

namespace metal {

class DataSink : public AbstractOperator {
public:
    explicit DataSink(size_t size) : _size(size) {}

    std::string id() const override { return ""; };
    uint8_t temp_enable_id() const override { return 0; /* Value is ignored: data sources and sinks are always enabled */ };
    uint8_t temp_stream_id() const override { return 0; };

protected:
    size_t _size;

};

class HostMemoryDataSink : public DataSink {
public:
    HostMemoryDataSink(void *dest, size_t size) : DataSink(size), _dest(dest) {}

    void configure(SnapAction &action) override;

protected:
    void *_dest;
};

class CardMemoryDataSink : public DataSink {
public:
    CardMemoryDataSink(uint64_t address, size_t size) : DataSink(size), _address(address) {}

    void configure(SnapAction &action) override;

protected:
    uint64_t _address;
};

class NullDataSink : public DataSink {
public:
    NullDataSink(size_t size) : DataSink(size) {}

    void configure(SnapAction &action) override;
};

} // namespace metal
