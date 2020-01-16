#include <unistd.h>

#include <snap_hls_if.h>

#include <snap_action_metal.h>
#include <iostream>
#include <metal-pipeline/data_source.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

RandomDataSource::RandomDataSource(size_t size) : DataSource(0, size) {
  _optionDefinitions.insert(std::make_pair(
      "length",
      OperatorOptionDefinition(
          0, OptionType::Uint, "length", "l",
          "The amount of data to generate, in bytes (default: 4096)")));

  _options.insert(std::make_pair("length", 4096u));
}

size_t RandomDataSource::reportTotalSize() {
  // TODO: Does this belong here? Maybe find a better method name then...
  _size = std::get<uint32_t>(_options.at("length").value());
  return _size;
}

}  // namespace metal
