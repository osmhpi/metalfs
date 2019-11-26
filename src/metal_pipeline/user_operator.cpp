#include "user_operator.hpp"

extern "C" {
#include <unistd.h>
#include <snap_hls_if.h>
}

#include "../../third_party/cxxopts/include/cxxopts.hpp"

#include <stdexcept>
#include <iostream>
#include <snap_action_metal.h>
#include <spdlog/spdlog.h>

#include "snap_action.hpp"

namespace metal {

UserOperator::UserOperator(std::string id, const std::string& manifest) : _manifest(jv_null()), _id(std::move(id)), _is_prepared(false) {

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

    // Build options object
    initializeOptions();
}

UserOperator::~UserOperator() {
    jv_free(_manifest);
}

void UserOperator::configure(SnapAction &action) {
    // Allocate job struct memory aligned on a page boundary, hopefully 4KB is sufficient
    auto job_config = reinterpret_cast<char*>(snap_malloc(4096));
    auto *metadata = reinterpret_cast<uint32_t*>(job_config);

    for (const auto& option : _options) {
        if (!option.second.has_value())
            continue;

        const auto &definition = _optionDefinitions.at(option.first);

        metadata[0] = htobe32(definition.offset() / sizeof(uint32_t)); // offset
        metadata[2] = htobe32(internal_id());
        metadata[3] = htobe32(prepare_required()); // enables preparation mode for the operator, implying that we need a preparation run of the operator(s)
        const int configuration_data_offset = 4 * sizeof(uint32_t); // bytes

        switch ((OptionType)option.second.value().index()) {
            case OptionType::Uint: {
                uint32_t value = std::get<uint32_t>(option.second.value());
                memcpy(job_config + configuration_data_offset, &value, sizeof(uint32_t));
                metadata[1] = htobe32(sizeof(uint32_t) / sizeof(uint32_t)); // length
                break;
            }
            case OptionType::Bool: {
                uint32_t value = std::get<bool>(option.second.value()) ? 1 : 0;
                memcpy(job_config + configuration_data_offset, &value, sizeof(uint32_t));
                metadata[1] = htobe32(1); // length
                break;
            }
            case OptionType::Buffer: {
                // *No* endianness conversions on buffers
                auto& buffer = *std::get<std::shared_ptr<std::vector<char>>>(option.second.value());
                memcpy(job_config + configuration_data_offset, buffer.data(), buffer.size());
                metadata[1] = htobe32(buffer.size() / sizeof(uint32_t)); // length
                break;
            }
        }

        _is_prepared = false;

        try {
            action.execute_job(fpga::JobType::ConfigureOperator, job_config);
        } catch (std::exception &ex) {
            // Something went wrong...
            spdlog::warn("Could not configure operator: {}", ex.what());
        }
    }

    free(job_config);
}

void UserOperator::finalize(SnapAction &action) {
    (void)action;
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

bool UserOperator::prepare_required() const {
    auto prepare = jv_object_get(manifest(), jv_string("prepare_required"));
    if (!jv_is_valid(prepare))
        return false;

    auto result = jv_equal(prepare, jv_true());
    jv_free(prepare);
    return result;
}

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
                _optionDefinitions.insert(std::make_pair(jv_string_value(key), OperatorOptionDefinition(
                    offset, OptionType::Bool, jv_string_value(key), shortK, description
                )));
            } else if (std::string("int") == jv_string_value(type)) {
                _optionDefinitions.insert(std::make_pair(jv_string_value(key), OperatorOptionDefinition(
                    offset, OptionType::Uint, jv_string_value(key), shortK, description
                )));
            }
        } else if (jv_get_kind(type) == JV_KIND_OBJECT) {
            std::string type_str = jv_string_value(jv_object_get(jv_copy(type), jv_string("type")));

            if (type_str == "buffer") {
                size_t size = jv_number_value(jv_object_get(jv_copy(type), jv_string("size")));
                _optionDefinitions.insert(std::make_pair(jv_string_value(key), OperatorOptionDefinition(
                    offset, OptionType::Buffer, jv_string_value(key), shortK, description, size
                )));
            }
        }

        _options.insert(std::make_pair(jv_string_value(key), std::nullopt));

        iter = jv_object_iter_next(options, iter);
    }

}

bool UserOperator::needs_preparation() const {
    return !_is_prepared && prepare_required();
}

} // namespace metal
