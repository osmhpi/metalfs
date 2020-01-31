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

uint64_t SnapPipelineRunner::run(DataSource dataSource, DataSink dataSink) {
  // One-off contexts for data source and data sink

  DefaultDataSourceContext source(dataSource);
  DefaultDataSinkContext sink(dataSink);

  return run(source, sink);
}

uint64_t SnapPipelineRunner::run(DataSourceContext &dataSource,
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
  dataSink.configure(action, dataSource.dataSource().address().size, initialize);

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
