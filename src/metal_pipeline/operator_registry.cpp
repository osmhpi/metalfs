#include "operator_registry.hpp"

extern "C" {
#include <jv.h>
}

#include <dirent.h>
#include <vector>
#include <spdlog/spdlog.h>

namespace metal {

OperatorRegistry::OperatorRegistry(const std::string &image_json) : _operators() {

    auto image = jv_parse(image_json.c_str());

    if (!jv_is_valid(image)) {
        jv_free(image);
        throw std::runtime_error("Error loading operator manifest");
    }

    if (jv_get_kind(image) != JV_KIND_OBJECT) {
        jv_free(image);
        throw std::runtime_error("Unexpected input");
    }

    auto operators = jv_object_get(image, jv_string("operators"));
    auto iter = jv_object_iter(operators);
    while (jv_object_iter_valid(operators, iter)) {
        auto key = jv_object_iter_key(operators, iter);
        auto current_operator = jv_object_iter_value(operators, iter);

        auto operator_string = jv_dump_string(current_operator, 0);
        auto op = std::make_unique<UserOperator>(jv_string_value(key), jv_string_value(operator_string));
        jv_free(operator_string);
        _operators.emplace(std::make_pair(op->id(), std::move(op)));

        iter = jv_object_iter_next(operators, iter);
    }

    jv_free(image);
}

} // namespace metal
