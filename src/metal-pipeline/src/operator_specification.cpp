#include <metal-pipeline/operator_specification.hpp>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace metal {

class UserOperatorSpecificationParser {
 public:
  UserOperatorSpecificationParser(const std::string& manifest)
      : _manifest(nlohmann::json::parse(manifest)) {
    // TODO: Validate manifest data (further...)
    if (!_manifest.is_object()) {
      throw std::runtime_error("Invalid manifest");
    }
  }

  std::string description() {
    return _manifest["description"].get<std::string>();
  }

  uint8_t streamID() { return _manifest["internal_id"].get<uint8_t>(); }

  bool prepareRequired() {
    // Optional attribute
    auto& prepare = _manifest["prepare_required"];
    if (prepare.is_null()) return false;
    return prepare.get<bool>();
  }

  std::unordered_map<std::string, OperatorOptionDefinition>
  optionDefinitions() {
    std::unordered_map<std::string, OperatorOptionDefinition> result;

    auto& options = _manifest["options"];

    // Optional attribute
    if (options.is_null()) return result;

    for (const auto& option : options.items()) {
      auto& key = option.key();
      auto& value = option.value();

      auto shortK = value["short"].get<std::string>();
      auto description = value["description"].get<std::string>();
      auto offset = value["offset"].get<uint64_t>();

      auto& type = value["type"];
      if (type.is_string()) {
        auto typeString = type.get<std::string>();
        if (typeString == "bool") {
          result.insert(std::make_pair(
              key, OperatorOptionDefinition(offset, OptionType::Bool, key,
                                            shortK, description)));
          continue;
        }
        if (typeString == "int") {
          result.insert(std::make_pair(
              key, OperatorOptionDefinition(offset, OptionType::Uint, key,
                                            shortK, description)));
          continue;
        }
      }
      if (type.is_object()) {
        auto typeString = type["type"].get<std::string>();
        if (typeString == "buffer") {
          auto size = type["size"].get<uint64_t>();
          result.insert(std::make_pair(
              key, OperatorOptionDefinition(offset, OptionType::Buffer, key,
                                            shortK, description, size)));
          continue;
        }
      }
      spdlog::warn("Unknown option declaration {} in operator.", key);
    }
    return result;
  }

 protected:
  nlohmann::json _manifest;
};

OperatorSpecification::OperatorSpecification(std::string id,
                                             const std::string& manifest)
    : _id(std::move(id)) {
  UserOperatorSpecificationParser parser(manifest);
  _description = parser.description();
  _streamID = parser.streamID();
  _prepareRequired = parser.prepareRequired();
  _optionDefinitions = parser.optionDefinitions();
}

}  // namespace metal
