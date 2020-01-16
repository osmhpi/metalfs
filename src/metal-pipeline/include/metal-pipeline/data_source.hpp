#pragma once

#include <metal-pipeline/metal-pipeline_api.h>
#include <metal-pipeline/abstract_operator.hpp>

#include <string>

#include <snap_action_metal.h>

namespace metal {

class METAL_PIPELINE_API DataSource : public AbstractOperator {
 public:
  explicit DataSource(uint64_t address, size_t size = 0)
      : _address(address), _size(size) {}

  std::string id() const override { return ""; }
  uint8_t internal_id() const override { return 0; }
  size_t size() const { return _size; }

  virtual size_t reportTotalSize() { return 0; }
  void setSize(size_t size) { _size = size; }

  fpga::Address address() {
    return fpga::Address{
        _address,
        static_cast<uint32_t>(_size),
        addressType(),
        mapType(),
    };
  }

 protected:
  virtual fpga::AddressType addressType() = 0;
  virtual fpga::MapType mapType() { return fpga::MapType::None; }
  uint64_t _address;
  size_t _size;
};

class METAL_PIPELINE_API HostMemoryDataSource : public DataSource {
 public:
  explicit HostMemoryDataSource(const void *source, size_t size = 0)
      : DataSource(reinterpret_cast<uint64_t>(source), size) {}
  void setSource(const void *source) {
    _address = reinterpret_cast<uint64_t>(source);
  }

 protected:
  fpga::AddressType addressType() override { return fpga::AddressType::Host; }
};

class METAL_PIPELINE_API CardMemoryDataSource : public DataSource {
 public:
  explicit CardMemoryDataSource(uint64_t address, size_t size = 0)
      : DataSource(address, size) {}
  void setAddress(uint64_t address) { _address = address; }

 protected:
  fpga::AddressType addressType() override {
    return fpga::AddressType::CardDRAM;
  }
};

class METAL_PIPELINE_API RandomDataSource : public DataSource {
 public:
  explicit RandomDataSource(size_t size = 0);

  std::string id() const override { return "datagen"; }
  std::string description() const override {
    return "Generate data on the FPGA for benchmarking operators.";
  }

  size_t reportTotalSize() override;

 protected:
  fpga::AddressType addressType() override { return fpga::AddressType::Random; }
};

}  // namespace metal
