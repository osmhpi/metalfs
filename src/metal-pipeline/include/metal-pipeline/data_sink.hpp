#pragma once

#include <metal-pipeline/metal-pipeline_api.h>

#include <string>

#include <snap_action_metal.h>

namespace metal {

class METAL_PIPELINE_API DataSink {
 public:
  explicit DataSink(uint64_t address, size_t size,
                    fpga::AddressType addressType,
                    fpga::MapType mapType = fpga::MapType::None)
      : _address(address),
        _size(size),
        _addressType(addressType),
        _mapType(mapType) {}
  explicit DataSink(void* address, size_t size)
      : DataSink(reinterpret_cast<uint64_t>(address), size,
                 fpga::AddressType::Host, fpga::MapType::None) {}

  virtual void prepareForTotalProcessingSize(size_t size) { (void)size; }
  virtual void setSize(size_t size) { _size = size; }

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
  size_t _size;
  fpga::AddressType _addressType;
  fpga::MapType _mapType;
};

}  // namespace metal
