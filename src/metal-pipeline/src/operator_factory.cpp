#include <metal-pipeline/operator_factory.hpp>

#include <dirent.h>

#include <vector>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <metal-pipeline/fpga_interface.hpp>
#include <metal-pipeline/operator_specification.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

OperatorFactory OperatorFactory::fromFPGA(SnapAction &snapAction) {
  uint64_t json_len = 0;
  auto json = snapAction.allocateMemory(4096);
  try {
    snapAction.executeJob(fpga::JobType::ReadImageInfo, json, {}, {}, 0, 0,
                          &json_len);
  } catch (std::exception &ex) {
    free(json);
    throw ex;
  }

  std::string info(reinterpret_cast<const char *>(json), json_len);
  free(json);

  return OperatorFactory(info);
}

OperatorFactory OperatorFactory::fromManifestString(
    const std::string &manifest) {
  return OperatorFactory(manifest);
}

OperatorFactory::OperatorFactory(const std::string &imageJson)
    : _operatorSpecifications() {
  auto image = nlohmann::json::parse(imageJson);

  if (!image.is_object()) {
    throw std::runtime_error("Unexpected input");
  }

  auto &operators = image["operators"];

  if (!operators.is_null()) {
    for (const auto &operatorEntry : image["operators"].items()) {
      auto &key = operatorEntry.key();
      auto &currentOperator = operatorEntry.value();

      auto operatorSpec =
          std::make_unique<OperatorSpecification>(key, currentOperator.dump());

      _operatorSpecifications.emplace(
          std::make_pair(operatorSpec->id(), std::move(operatorSpec)));
    }
  }

  _isDRAMEnabled = image["target"]["dram"].get<bool>();
  _isNVMeEnabled = image["target"]["nvme"].get<bool>();
}

Operator OperatorFactory::createOperator(std::string id) {
  return Operator(_operatorSpecifications.at(id));
}

}  // namespace metal
