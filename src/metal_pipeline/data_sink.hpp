#pragma once

#include <string>
#include "abstract_operator.hpp"

namespace metal {

class DataSink : public AbstractOperator {
public:
    explicit DataSink(size_t size = 0) : _size(size) {}

    std::string id() const override { return ""; };
    uint8_t temp_enable_id() const override { return 0; /* Value is ignored: data sources and sinks are always enabled */ };
    uint8_t temp_stream_id() const override { return 0; };

    virtual void prepareForTotalProcessingSize(size_t size) { (void)size; }
    virtual void setSize(size_t size);

protected:
    size_t _size;

};

class HostMemoryDataSink : public DataSink {
public:
    explicit HostMemoryDataSink(void *dest, size_t size = 0) : DataSink(size), _dest(dest) {}

    void configure(SnapAction &action) override;
    void finalize(SnapAction &action) override { (void) action; };

protected:
    void *_dest;
};

class CardMemoryDataSink : public DataSink {
public:
    explicit CardMemoryDataSink(uint64_t address, size_t size = 0) : DataSink(size), _address(address) {}

    void configure(SnapAction &action) override;
    void finalize(SnapAction &action) override { (void)action; }
    void setAddress(uint64_t address) { _address = address; }

protected:
    uint64_t _address;
};

class NullDataSink : public DataSink {
public:
    explicit NullDataSink(size_t size = 0) : DataSink(size) {}

    void configure(SnapAction &action) override;
    void finalize(SnapAction& action) override { (void)action; }
};

} // namespace metal
