#include "base_test.hpp"

namespace metal {
void BaseTest::SetUp() {

    char *testPath = std::getenv("TEST_PATH");
    std::string testDataBasePath;
    if (testPath != nullptr)
        testDataBasePath = std::string(testPath);

    std::string operatorsPath;
    if (testDataBasePath.empty()) {
        operatorsPath = "./test/metal_pipeline_test/operators";
    } else {
        operatorsPath = testDataBasePath;
        operatorsPath.append("/test/metal_pipeline_test/operators");
    }

    _registry = std::make_unique<OperatorRegistry>(operatorsPath);
}

void BaseTest::TearDown() {

}

} // namespace metal
