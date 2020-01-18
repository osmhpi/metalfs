#include <metal-pipeline/snap_pipeline_runner.hpp>

extern "C" {
#include <unistd.h>
}

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

#include <snap_action_metal.h>
#include <metal-pipeline/common.hpp>
#include <metal-pipeline/pipeline_definition.hpp>
#include <metal-pipeline/snap_action.hpp>
#include <metal-pipeline/user_operator_specification.hpp>

namespace metal {

std::string SnapPipelineRunner::readImageInfo(int card) {
  SnapAction action(fpga::ActionType, card);
  uint64_t json_len = 0;
  auto json = action.allocateMemory(4096);
  try {
    action.execute_job(fpga::JobType::ReadImageInfo, json, {}, {}, 0, 0,
                       &json_len);
  } catch (std::exception &ex) {
    free(json);
    throw ex;
  }

  std::string result(reinterpret_cast<const char *>(json), json_len);
  free(json);
  return result;
}

uint64_t SnapPipelineRunner::run(DataSource dataSource, DataSink dataSink) {
  // One-off contexts for data source and data sink

  DefaultDataSourceRuntimeContext source(dataSource);
  DefaultDataSinkRuntimeContext sink(dataSink);

  return run(source, sink);
}

uint64_t SnapPipelineRunner::run(DataSourceRuntimeContext &dataSource,
                                 DataSinkRuntimeContext &dataSink) {
  SnapAction action = SnapAction(fpga::ActionType, _card);

  auto initialize = !_initialized;

  if (initialize) {
    auto totalSize = dataSource.reportTotalSize();
    if (totalSize > 0) {
      dataSink.prepareForTotalSize(totalSize);
    }

    _pipeline->configureSwitch(action, true);
  }

  dataSource.configure(action, initialize);
  dataSink.configure(action, initialize);

  _initialized = true;

  preRun(action, dataSource, dataSink, initialize);
  uint64_t output_size =
      _pipeline->run(dataSource.dataSource(), dataSink.dataSink(), action);
  postRun(action, dataSource, dataSink, dataSource.endOfInput());

  dataSource.finalize(action);
  dataSink.finalize(action, output_size, dataSource.endOfInput());

  return output_size;
}

}  // namespace metal
