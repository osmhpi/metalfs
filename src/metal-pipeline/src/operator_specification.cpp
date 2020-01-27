#include <metal-pipeline/operator_specification.hpp>

extern "C" {
#include <jv.h>
}

namespace metal {

class UserOperatorSpecificationParser {
 public:
  UserOperatorSpecificationParser(const std::string& manifest)
      : _manifest(jv_null()) {
    // Memory management with jq is a little cumbersome.
    // jv.h says:
    //    All jv_* functions consume (decref) input and produce (incref) output
    //    Except jv_copy
    // ... and jv_*_value, and jv_get_kind, ...

    // TODO: Validate manifest data

    auto m = jv_parse(manifest.c_str());

    if (jv_get_kind(m) != JV_KIND_OBJECT) {
      jv_free(m);
      throw std::runtime_error("Unexpected input");
    }

    _manifest = m;
  }

  ~UserOperatorSpecificationParser() { jv_free(_manifest); }

  std::string description() {
    auto desc = jv_object_get(manifest(), jv_string("description"));
    std::string result = jv_string_value(desc);
    jv_free(desc);
    return result;
  }

  uint8_t streamID() {
    auto jv_internal_id = jv_object_get(manifest(), jv_string("internal_id"));
    int internal_id = (int)jv_number_value(jv_internal_id);
    jv_free(jv_internal_id);
    return internal_id;
  }

  bool prepareRequired() {
    auto prepare = jv_object_get(manifest(), jv_string("prepare_required"));
    if (!jv_is_valid(prepare)) return false;

    auto result = jv_equal(prepare, jv_true());
    jv_free(prepare);
    return result;
  }

  std::unordered_map<std::string, OperatorOptionDefinition>
  optionDefinitions() {
    std::unordered_map<std::string, OperatorOptionDefinition> result;

    auto options = jv_object_get(manifest(), jv_string("options"));

    auto iter = jv_object_iter(options);
    while (jv_object_iter_valid(options, iter)) {
      auto key = jv_object_iter_key(options, iter);
      auto option = jv_object_iter_value(options, iter);

      std::string shortK;
      {
        auto shortKey = jv_object_get(jv_copy(option), jv_string("short"));
        if (jv_get_kind(shortKey) != JV_KIND_INVALID)
          shortK = jv_string_value(shortKey);
        jv_free(shortKey);
      }

      auto desc = jv_object_get(jv_copy(option), jv_string("description"));
      std::string description = jv_string_value(desc);
      jv_free(desc);

      auto off = jv_object_get(jv_copy(option), jv_string("offset"));
      int offset = (int)jv_number_value(off);
      jv_free(off);

      auto type = jv_object_get(jv_copy(option), jv_string("type"));
      if (jv_get_kind(type) == JV_KIND_STRING) {
        if (std::string("bool") == jv_string_value(type)) {
          result.insert(
              std::make_pair(jv_string_value(key),
                             OperatorOptionDefinition(offset, OptionType::Bool,
                                                      jv_string_value(key),
                                                      shortK, description)));
        } else if (std::string("int") == jv_string_value(type)) {
          result.insert(
              std::make_pair(jv_string_value(key),
                             OperatorOptionDefinition(offset, OptionType::Uint,
                                                      jv_string_value(key),
                                                      shortK, description)));
        }
      } else if (jv_get_kind(type) == JV_KIND_OBJECT) {
        std::string type_str =
            jv_string_value(jv_object_get(jv_copy(type), jv_string("type")));

        if (type_str == "buffer") {
          size_t size =
              jv_number_value(jv_object_get(jv_copy(type), jv_string("size")));
          result.insert(std::make_pair(
              jv_string_value(key),
              OperatorOptionDefinition(offset, OptionType::Buffer,
                                       jv_string_value(key), shortK,
                                       description, size)));
        }
      }

      iter = jv_object_iter_next(options, iter);
    }
    return result;
  }

 protected:
  jv manifest() const { return jv_copy(_manifest); }
  jv _manifest;
};

OperatorSpecification::OperatorSpecification(
    std::string id, const std::string& manifest)
    : _id(std::move(id)) {
  UserOperatorSpecificationParser parser(manifest);
  _description = parser.description();
  _streamID = parser.streamID();
  _prepareRequired = parser.prepareRequired();
  _optionDefinitions = parser.optionDefinitions();
}

}  // namespace metal
