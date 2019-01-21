extern "C" {
#include <jv.h>
}

#include <stdexcept>
#include <iostream>
#include "user_operator.hpp"

namespace metal {

UserOperator::UserOperator(std::string manifest_path) {

    // Memory management with jq is a little cumbersome.
    // jv.h says:
    //    All jv_* functions consume (decref) input and produce (incref) output
    //    Except jv_copy
    // ... and jv_*_value, and jv_get_kind, ...

    jv data = jv_load_file(manifest_path.c_str(), /* raw = */ false);

    if (!jv_is_valid(data)) {
        jv_free(data);
        throw std::runtime_error("Error loading operator manifest");
    }

    if (jv_get_kind(data) != JV_KIND_ARRAY) {
        jv_free(data);
        throw std::runtime_error("Unexpected input");
    }

    jv manifest = jv_array_get(data, 0);

    // TODO: Validate manifest data

    if (jv_get_kind(manifest) != JV_KIND_OBJECT) {
        jv_free(manifest);
        throw std::runtime_error("Unexpected input");
    }

//    jv_show(manifest, -1);

    _manifest = manifest;
}

UserOperator::~UserOperator() {
    jv_free(_manifest);
}

void UserOperator::parseArguments() {

}

void UserOperator::configure() {

}

void UserOperator::finalize() {

}

std::string UserOperator::id() const {
    auto temp = jv_object_get(manifest(), jv_string("id"));
    std::string result = jv_string_value(temp);
    jv_free(temp);
    return result;
}

void UserOperator::setOption(std::string option, OperatorArgument arg) {

    // Get option handle
    auto options = jv_object_get(manifest(), jv_string("options"));
    auto o = jv_object_get(options, jv_string(option.c_str()));

    if (jv_get_kind(jv_copy(o)) == JV_KIND_INVALID) {
        jv_free(o);
        throw std::runtime_error("Invalid option key");
    }

    auto type = jv_object_get(o, jv_string("type"));

    bool valid = true;

    switch (arg.index()) {
        case 0: { // int
            if (jv_get_kind(type) != JV_KIND_STRING || std::string("int") != jv_string_value(type))
                valid = false;
            break;
        }
        case 1: { // bool
            if (jv_get_kind(type) != JV_KIND_STRING || std::string("bool") != jv_string_value(type))
                valid = false;
            break;
        }
        case 2: { // buffer
            auto internalType = jv_object_get(jv_copy(type), jv_string("type"));
            if (jv_get_kind(type) != JV_KIND_OBJECT || std::string("buffer") != jv_string_value(internalType))
                valid = false;
            jv_free(internalType);

            if (!valid)
                break;

            auto &buffer = std::get<std::unique_ptr<std::vector<char>>>(arg);
            auto size = jv_object_get(jv_copy(type), jv_string("size"));
            if ((int)jv_number_value(size) != buffer->size())
                valid = false;
            jv_free(size);

            break;
        }
        default:
            throw std::runtime_error("Not implemented");
    }

    jv_free(type);

    if (!valid) {
        throw std::runtime_error("Invalid option");
    }

    _options[option] = std::move(arg);
}

} // namespace metal
