#include "base_test.hpp"

#include <libgen.h>

namespace metal {

void PipelineTest::SetUp() {

    // Determine the path to the operators directory

    std::vector<char> buf(1024, 0);
    ssize_t result = readlink("/proc/self/exe", &buf[0], buf.size());
    if (result < 0)
    {
        throw std::runtime_error("Could not read /proc/self/exe");
    }

    const char *dir = dirname(buf.data());

    // We assume that the executable resides at metal_fs/<cmake_build>/example_test
    auto operatorsPath = std::string(dir) + "/../example/image/operators";

    std::cout << operatorsPath << std::endl;

    _registry = std::make_unique<OperatorRegistry>(operatorsPath);
}

} // namespace metal
