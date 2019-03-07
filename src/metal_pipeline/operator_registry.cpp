#include "operator_registry.hpp"

#include <dirent.h>
#include <vector>

namespace metal {

OperatorRegistry::OperatorRegistry(const std::string search_path)
    : _operators() {

    std::vector<std::string> files_in_search_path;

    {
        struct dirent *dp = nullptr;
        auto dirp = opendir(search_path.c_str());
        while ((dp = readdir(dirp)) != nullptr) {
            if (dp->d_type == DT_REG)
                files_in_search_path.emplace_back(std::string(dp->d_name));
        }
        closedir(dirp);
    }

    for (const auto & filename : files_in_search_path) {
        const std::string jsonFileEnding = ".json";

        // if (!filename.ends_with(jsonFileEnding)):
        if (!(filename.size() >= jsonFileEnding.size() && filename.compare(filename.size() - jsonFileEnding.size(), jsonFileEnding.size(), jsonFileEnding) == 0))
            continue;

        try {
            auto full_path = search_path;
            full_path.append("/");
            full_path.append(filename);

            auto op = std::make_unique<UserOperator>(full_path);
            _operators.emplace(std::make_pair(op->id(), std::move(op)));
        } catch (std::exception &ex) {
            // Could not load operator
        }
    }
}

} // namespace metal
