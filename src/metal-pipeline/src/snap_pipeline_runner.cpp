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
#include <metal-pipeline/data_sink_context.hpp>
#include <metal-pipeline/data_source_context.hpp>
#include <metal-pipeline/operator_specification.hpp>
#include <metal-pipeline/pipeline.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

std::pair<uint64_t, bool> SnapPipelineRunner::run(DataSource dataSource,
                                                  DataSink dataSink) {
  // One-off contexts for data source and data sink

  DefaultDataSourceContext source(dataSource);
  DefaultDataSinkContext sink(dataSink);

  return run(source, sink);
}

std::pair<uint64_t, bool> SnapPipelineRunner::run(DataSourceContext &dataSource,
                                                  DataSinkContext &dataSink) {
  SnapAction action = SnapAction(_card);

  auto initialize = !_initialized;

  if (initialize) {
    auto totalSize = dataSource.reportTotalSize();
    if (totalSize > 0) {
      dataSink.prepareForTotalSize(totalSize);
    }

    _pipeline->configureSwitch(action, true);
  }

  dataSource.configure(action, initialize);
  dataSink.configure(action, dataSource.dataSource().address().size,
                     initialize);

  _initialized = true;
  auto endOfInput = dataSource.endOfInput();

  preRun(action, dataSource, dataSink, initialize);
  auto outputSize =
      _pipeline->run(dataSource.dataSource(), dataSink.dataSink(), action);
  postRun(action, dataSource, dataSink, endOfInput);

  dataSource.finalize(action);
  dataSink.finalize(action, outputSize, endOfInput);

  return std::make_pair(outputSize, endOfInput);
}

}  // namespace metal
