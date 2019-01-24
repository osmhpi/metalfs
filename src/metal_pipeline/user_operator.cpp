#include "user_operator.hpp"

extern "C" {
#include <unistd.h>
#include <snap_hls_if.h>
#include <jv.h>
}

#include "../../third_party/cxxopts/include/cxxopts.hpp"

#include <stdexcept>
#include <iostream>

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

//    jv_show(manifest, -1);
    _manifest = manifest;

    // Build options object
    _opts = std::make_unique<cxxopts::Options>(id(), description());
    initializeOptions();
}

UserOperator::~UserOperator() {
    jv_free(_manifest);
}

void UserOperator::parseArguments(std::vector<std::string> args) {
    // Contain some C++ / C interop ugliness inside here...

    std::vector<char*> argsRaw;
    for (const auto &arg : args)
        argsRaw.emplace_back(const_cast<char*>(arg.c_str()));

    int argc = (int)args.size();
    char **argv = argsRaw.data();
    auto parseResult = _opts->parse(argc, argv);

    for (const auto &argument : parseResult.arguments()) {
        auto optionType = _optionTypes.find(argument.key());
        if (optionType == _optionTypes.end())
            throw std::runtime_error("Option was not found");

        switch (optionType->second) {
            case OptionType::INT: {
                setOption(argument.key(), argument.as<int>());
                break;
            }
            case OptionType::BOOL: {
                setOption(argument.key(), argument.as<bool>());
                break;
            }
            case OptionType::BUFFER: {
                auto fileOption = _fileOptions.find(argument.key());
                if (fileOption == _fileOptions.end())
                    throw std::runtime_error("File option metadata not found");

                auto fake = std::make_unique<std::vector<char>>(fileOption->second);
                setOption(argument.key(), std::move(fake));
            }
        }
    }
}

void UserOperator::configure(SnapAction &action) {
    AbstractOperator::configure(action);

    auto jv_config_job_id = jv_object_get(manifest(), jv_string("temp_config_job_id"));
    int config_job_id = (int)jv_number_value(jv_config_job_id);
    jv_free(jv_config_job_id);

    // Allocate job struct memory aligned on a page boundary, hopefully 4KB is sufficient
    char *job_config = (char*)snap_malloc(4096);

    for (const auto& option : _options) {
        if (!option.second.hasValue())
            continue;

        switch ((OptionType)option.second.value().index()) {
            case OptionType::INT: {
                int beValue = htobe64(std::get<int>(option.second.value()));
                // Accessing un-validated, user-provided offset: dangerous!
                memcpy(job_config + option.second.offset(), &beValue, sizeof(int));
                break;
            }
            case OptionType::BOOL: {
                // Is htobe(bool) well-defined?
                int beValue = htobe64(std::get<bool>(option.second.value()));
                memcpy(job_config + option.second.offset(), &beValue, sizeof(int));
                break;
            }
            case OptionType::BUFFER: {
                // *No* endianness conversions on buffers
                auto& buffer = *std::get<std::shared_ptr<std::vector<char>>>(option.second.value());
                memcpy(job_config + option.second.offset(), buffer.data(), buffer.size());
                break;
            }
        }
    }

    try {
        action.execute_job(static_cast<uint64_t>(config_job_id), job_config);
    } catch (std::exception &ex) {
        // Something went wrong...
    };

    free(job_config);
}

void UserOperator::finalize(SnapAction &action) {
    AbstractOperator::finalize(action);
}

std::string UserOperator::id() const {
    auto jv_id = jv_object_get(manifest(), jv_string("id"));
    std::string result = jv_string_value(jv_id);
    jv_free(jv_id);
    return result;
}

uint8_t UserOperator::temp_stream_id() const {
    auto jv_stream_id = jv_object_get(manifest(), jv_string("temp_stream_id"));
    int stream_id = (int)jv_number_value(jv_stream_id);
    jv_free(jv_stream_id);
    return stream_id;
}

uint8_t UserOperator::temp_enable_id() const {
    auto jv_enable_id = jv_object_get(manifest(), jv_string("temp_enable_id"));
    int enable_id = (int)jv_number_value(jv_enable_id);
    jv_free(jv_enable_id);
    return enable_id;
}

std::string UserOperator::description() const {
    auto desc = jv_object_get(manifest(), jv_string("description"));
    std::string result = jv_string_value(desc);
    jv_free(desc);
    return result;
}

void UserOperator::setOption(std::string option, OperatorArgumentValue arg) {

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

    if (_optionTypes.at(option) != (OptionType)arg.index()) {
        throw std::runtime_error("Invalid option");
    }

    _options.at(option).setValue(std::move(arg));
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
                _opts->add_option("", shortK, jv_string_value(key), description, cxxopts::value<bool>(), "");
                _optionTypes[jv_string_value(key)] = OptionType::BOOL;
                _options[jv_string_value(key)] = OperatorArgument(offset);
            } else if (std::string("int") == jv_string_value(type)) {
                _opts->add_option("", shortK, jv_string_value(key), description, cxxopts::value<int>(), "");
                _optionTypes[jv_string_value(key)] = OptionType::INT;
                _options[jv_string_value(key)] = OperatorArgument(offset);
            }
        } else {
            // Buffer (path to file)
            _opts->add_option("", shortK, jv_string_value(key), description, cxxopts::value<std::string>(), "");
            _optionTypes[jv_string_value(key)] = OptionType::BUFFER;
            _options[jv_string_value(key)] = OperatorArgument(offset);
        }

        iter = jv_object_iter_next(options, iter);
    }

}

} // namespace metal
