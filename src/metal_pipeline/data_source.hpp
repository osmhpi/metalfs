#pragma once

#include <string>
#include "abstract_operator.hpp"

namespace metal {

class DataSource : public AbstractOperator {
public:
    explicit DataSource(size_t size = 0) : _size(size) {}

    std::string id() const override { return ""; };
    uint8_t internal_id() const override { return 0; /* Value is ignored: data sources and sinks are always enabled */ };
    size_t size() const { return _size; }

    virtual size_t reportTotalSize() { return 0; }
    void setSize(size_t size);

protected:
    size_t _size;

};

class HostMemoryDataSource : public DataSource {
public:
    explicit HostMemoryDataSource(const void *dest, size_t size = 0) : DataSource(size), _dest(dest) {}

    void configure(SnapAction &action) override;
    void finalize(SnapAction &action) override { (void) action; };

protected:
    const void *_dest;
};

class CardMemoryDataSource : public DataSource {
public:
    explicit CardMemoryDataSource(uint64_t address, size_t size = 0) : DataSource(size), _address(address) {}

    void configure(SnapAction &action) override;
    void finalize(SnapAction &action) override { (void)action; }
    void setAddress(uint64_t address) { _address = address; }

protected:
    uint64_t _address;
};

class RandomDataSource : public DataSource {
public:
    explicit RandomDataSource(size_t size = 0) : DataSource(size) {}

    void configure(SnapAction &action) override;
    void finalize(SnapAction& action) override { (void)action; }
};

} // namespace metal
