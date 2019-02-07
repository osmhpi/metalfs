#pragma once

#include <string>
#include "abstract_operator.hpp"

namespace metal {

class DataSource : public AbstractOperator {
public:
    explicit DataSource(size_t size) : _size(size) {}

    std::string id() const override { return ""; };
    uint8_t temp_enable_id() const override { return 0; /* Value is ignored: data sources and sinks are always enabled */ };
    uint8_t temp_stream_id() const override { return 0; };
    size_t size() const { return _size; }

protected:
    size_t _size;

};

class HostMemoryDataSource : public DataSource {
public:
    HostMemoryDataSource(void *dest, size_t size) : DataSource(size), _dest(dest) {}

    void configure(SnapAction &action) override;

protected:
    void *_dest;
};

class CardMemoryDataSource : public DataSource {
public:
    CardMemoryDataSource(uint64_t address, size_t size) : DataSource(size), _address(address) {}

    void configure(SnapAction &action) override;

protected:
    uint64_t _address;
};

class RandomDataSource : public DataSource {
public:
    RandomDataSource(size_t size) : DataSource(size) {}

    void configure(SnapAction &action) override;
};

} // namespace metal
