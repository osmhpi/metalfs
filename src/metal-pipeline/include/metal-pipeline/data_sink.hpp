#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>

#include <metal-pipeline/fpga_interface.hpp>

namespace metal {

class METAL_PIPELINE_API DataSink {
 public:
  explicit DataSink(uint64_t address, uint64_t size,
                    fpga::AddressType addressType,
                    fpga::MapType mapType = fpga::MapType::None)
      : _address(address),
        _size(size),
        _addressType(addressType),
        _mapType(mapType) {}
  explicit DataSink(void* address, uint64_t size)
      : DataSink(reinterpret_cast<uint64_t>(address), size,
                 fpga::AddressType::Host, fpga::MapType::None) {}

  DataSink withSize(uint64_t newSize) {
    return DataSink(_address, newSize, _addressType, _mapType);
  }

  fpga::Address address() const {
    return fpga::Address{
        _address,
        static_cast<uint32_t>(_size),
        _addressType,
        _mapType,
    };
  }

 protected:
  uint64_t _address;
  uint64_t _size;
  fpga::AddressType _addressType;
  fpga::MapType _mapType;
};

}  // namespace metal
