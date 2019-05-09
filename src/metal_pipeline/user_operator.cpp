#include "user_operator.hpp"

extern "C" {
#include <unistd.h>
#include <snap_hls_if.h>
#include <jv.h>
}

#include "../../third_party/cxxopts/include/cxxopts.hpp"

#include <stdexcept>
#include <iostream>
#include <metal_fpga/hw/hls/include/action_metalfpga.h>

#include "snap_action.hpp"

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

    _manifest = manifest;

    // Build options object
    initializeOptions();
}

UserOperator::~UserOperator() {
    jv_free(_manifest);
}

void UserOperator::configure(SnapAction &action) {
    int config_job_id = MTL_JOB_OP_CONFIGURE;
    const int configuration_data_offset = 4 * sizeof(uint32_t); // bytes

    // Allocate job struct memory aligned on a page boundary, hopefully 4KB is sufficient
    auto job_config = reinterpret_cast<char*>(snap_malloc(4096));

    for (const auto& option : _options) {
        if (!option.second.has_value())
            continue;

        const auto &definition = _optionDefinitions.at(option.first);

        auto *metadata = reinterpret_cast<uint32_t*>(job_config);
        metadata[0] = htobe32(definition.offset() / sizeof(uint32_t)); // offset
        metadata[2] = htobe32(internal_id());

        switch ((OptionType)option.second.value().index()) {
            case OptionType::INT: {
                int beValue = htobe64(std::get<int>(option.second.value()));
                // Accessing un-validated, user-provided offset: dangerous!
                memcpy(job_config + configuration_data_offset, &beValue, sizeof(int));
                metadata[1] = htobe32(sizeof(int)/ sizeof(uint32_t)); // length
                break;
            }
            case OptionType::BOOL: {
                // Is htobe(bool) well-defined?
                int beValue = htobe64(std::get<bool>(option.second.value()));
                memcpy(job_config + configuration_data_offset, &beValue, sizeof(int));
                metadata[1] = htobe32(sizeof(int)/ sizeof(uint32_t)); // length
                break;
            }
            case OptionType::BUFFER: {
                // *No* endianness conversions on buffers
                auto& buffer = *std::get<std::shared_ptr<std::vector<char>>>(option.second.value());
                memcpy(job_config + configuration_data_offset, buffer.data(), buffer.size());
                metadata[1] = htobe32(buffer.size() / sizeof(uint32_t)); // length
                break;
            }
        }

        try {
            action.execute_job(static_cast<uint64_t>(config_job_id), job_config);
        } catch (std::exception &ex) {
            // Something went wrong...
        }
    }

    free(job_config);
}

void UserOperator::finalize(SnapAction &action) {
    (void)action;
}

std::string UserOperator::id() const {
    auto jv_id = jv_object_get(manifest(), jv_string("id"));
    std::string result = jv_string_value(jv_id);
    jv_free(jv_id);
    return result;
}

uint8_t UserOperator::internal_id() const {
    auto jv_internal_id = jv_object_get(manifest(), jv_string("internal_id"));
    int internal_id = (int)jv_number_value(jv_internal_id);
    jv_free(jv_internal_id);
    return internal_id;
}

std::string UserOperator::description() const {
    auto desc = jv_object_get(manifest(), jv_string("description"));
    std::string result = jv_string_value(desc);
    jv_free(desc);
    return result;
}

//void UserOperator::setOption(std::string option, OperatorArgumentValue arg) {

    // Get option handle
//    auto options = jv_object_get(manifest(), jv_string("options"));
//    auto o = jv_object_get(options, jv_string(option.c_str()));
//
//    if (jv_get_kind(jv_copy(o)) == JV_KIND_INVALID) {
//        jv_free(o);
//        throw std::runtime_error("Invalid option key");
//    }
//
//    auto type = jv_object_get(o, jv_string("type"));

//    bool valid = true;
//
//    switch ((OptionType)arg.index()) {
//        case OptionType::INT: {
//            if (_optionTypes.at(option) != O)
//                valid = false;
//            break;
//        }
//        case OptionType::BOOL: {
//            if (jv_get_kind(type) != JV_KIND_STRING || std::string("bool") != jv_string_value(type))
//                valid = false;
//            break;
//        }
//        case OptionType::BUFFER: {
//            auto internalType = jv_object_get(jv_copy(type), jv_string("type"));
//            if (jv_get_kind(type) != JV_KIND_OBJECT || std::string("buffer") != jv_string_value(internalType))
//                valid = false;
//            jv_free(internalType);
//
//            if (!valid)
//                break;
//
//            auto &buffer = std::get<std::unique_ptr<std::vector<char>>>(arg);
//            auto size = jv_object_get(jv_copy(type), jv_string("size"));
//            if ((int)jv_number_value(size) != buffer->size())
//                valid = false;
//            jv_free(size);
//
//            break;
//        }
//        default:
//            throw std::runtime_error("Not implemented");
//    }
//
//    jv_free(type);

//    if (_optionTypes.at(option) != (OptionType)arg.index()) {
//        throw std::runtime_error("Invalid option");
//    }
//
//    _options.at(option).setValue(std::move(arg));
//}

void UserOperator::initializeOptions() {

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
                _optionDefinitions[jv_string_value(key)] = OperatorOptionDefinition(
                    offset, OptionType::BOOL, jv_string_value(key), shortK, description
                );
            } else if (std::string("int") == jv_string_value(type)) {
                _optionDefinitions[jv_string_value(key)] = OperatorOptionDefinition(
                    offset, OptionType::INT, jv_string_value(key), shortK, description
                );
            }
        } else {
            // Buffer (path to file)
            _optionDefinitions[jv_string_value(key)] = OperatorOptionDefinition(
                offset, OptionType::BUFFER, jv_string_value(key), shortK, description
            );
        }

        _options[jv_string_value(key)] = std::nullopt;

        iter = jv_object_iter_next(options, iter);
    }

}

} // namespace metal
