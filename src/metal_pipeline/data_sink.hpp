#pragma once

#include "abstract_operator.hpp"

#include <string>

#include <metal_fpga/hw/hls/include/snap_action_metal.h>

namespace metal {

class DataSink : public AbstractOperator {
public:
    explicit DataSink(uint64_t address, size_t size = 0) : _address(address), _size(size) {}

    std::string id() const override { return ""; }
    uint8_t internal_id() const override { return 0; }

    virtual void prepareForTotalProcessingSize(size_t size) { (void)size; }
    virtual void setSize(size_t size) { _size = size; }

    fpga::Address address() {
        return fpga::Address {
            _address,
            static_cast<uint32_t>(_size),
            addressType(),
            0 /* padding */
        };
    }

protected:
    virtual fpga::AddressType addressType() = 0;
    uint64_t _address;
    size_t _size;

};

class HostMemoryDataSink : public DataSink {
public:
    explicit HostMemoryDataSink(void *destination, size_t size = 0) : DataSink(reinterpret_cast<uint64_t>(destination), size) {}
    void setDestination(void *destination) { _address = reinterpret_cast<uint64_t>(destination); }

protected:
    fpga::AddressType addressType() override { return fpga::AddressType::Host; }
};

class CardMemoryDataSink : public DataSink {
public:
    explicit CardMemoryDataSink(uint64_t address, size_t size = 0) : DataSink(address, size) {}
    void setAddress(uint64_t address) { _address = address; }

protected:
    fpga::AddressType addressType() override { return fpga::AddressType::CardDRAM; }
};

class NullDataSink : public DataSink {
public:
    explicit NullDataSink(size_t size = 0) : DataSink(0, size) {}
protected:
    fpga::AddressType addressType() override { return fpga::AddressType::Null; }
};

} // namespace metal
