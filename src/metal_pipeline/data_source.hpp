#pragma once

#include "abstract_operator.hpp"

#include <string>

#include <metal_fpga/hw/hls/include/snap_action_metal.h>

namespace metal {

class DataSource : public AbstractOperator {
public:
    explicit DataSource(uint64_t address, size_t size = 0) : _address(address), _size(size) {}

    std::string id() const override { return ""; }
    uint8_t internal_id() const override { return 0; }
    size_t size() const { return _size; }

    virtual size_t reportTotalSize() { return 0; }
    void setSize(size_t size) { _size = size; }

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

class HostMemoryDataSource : public DataSource {
public:
    explicit HostMemoryDataSource(const void *source, size_t size = 0) : DataSource(reinterpret_cast<uint64_t>(source), size) {}
    void setSource(const void *source) { _address = reinterpret_cast<uint64_t>(source); }

protected:
    fpga::AddressType addressType() override { return fpga::AddressType::Host; }
};

class CardMemoryDataSource : public DataSource {
public:
    explicit CardMemoryDataSource(uint64_t address, size_t size = 0) : DataSource(address, size) {}
    void setAddress(uint64_t address) { _address = address; }

protected:
    fpga::AddressType addressType() override { return fpga::AddressType::CardDRAM; }
};

class RandomDataSource : public DataSource {
public:
    explicit RandomDataSource(size_t size = 0);

    std::string id() const override { return "datagen"; }
    std::string description() const override { return "Generate data on the FPGA for benchmarking operators."; }

    size_t reportTotalSize() override;
protected:
    fpga::AddressType addressType() override { return fpga::AddressType::Random; }
};

} // namespace metal
