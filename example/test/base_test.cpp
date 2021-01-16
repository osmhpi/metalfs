#include "base_test.hpp"

#include <libgen.h>

#ifdef WITH_SNAP
#include <metal-pipeline/snap/snap_action.hpp>
#elif WITH_OCACCEL
#include <metal-pipeline/ocaccel/ocaccel_action.hpp>
#endif

namespace metal {

BaseTest::BaseTest() :
  #ifdef WITH_SNAP
    _actionFactory(std::make_shared<SnapActionFactory>(0, 10))
  #elif WITH_OCACCEL
    _actionFactory(std::make_shared<OCAccelActionFactory>(0, 10))
  #endif
    {}

void BaseTest::fill_payload(uint8_t *buffer, uint64_t length) {
  for (uint64_t i = 0; i < length; ++i) {
    buffer[i] = i % 256;
  }
}

void BaseTest::SetUp() {
#ifndef __PPC64__
  GTEST_SKIP();
#endif
  _setUp();
}

void PipelineTest::_setUp() {
  auto action = _actionFactory->createAction();
  _registry = std::make_unique<OperatorFactory>(OperatorFactory::fromFPGA(*action));
}

std::optional<Operator> PipelineTest::try_get_operator(
    const std::string &key) {
  if (_registry->operatorSpecifications().find(key) ==
      _registry->operatorSpecifications().end())
    return std::nullopt;

  return _registry->createOperator(key);
}

void SimulationPipelineTest::SetUp() { _setUp(); }

void SimulationTest::SetUp() { _setUp(); }

}  // namespace metal
