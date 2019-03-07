
#include "filesystem_pipeline_runner.hpp"
#include "file_data_source.hpp"
#include "file_data_sink.hpp"

namespace metal {

//void metal::FilesystemPipelineRunner::initialize(SnapAction &action) {
//    ProfilingPipelineRunner::initialize(action);
//
//    auto fileDataSource = std::dynamic_pointer_cast<FileDataSource>(_pipeline->operators().front());
//    auto fileDataSink = std::dynamic_pointer_cast<FileDataSink>(_pipeline->operators().back());
//
//    if (fileDataSource && fileDataSink) {
//        // When reading from internal input file, we know upfront how big the output file will be
//        fileDataSink->ensureFileAllocation(fileDataSource->file_size());
//    } else if (fileDataSink) {
//        auto dataSource = std::dynamic_pointer_cast<DataSource>(_pipeline->operators().front());
//        fileDataSink->ensureFileAllocation(dataSource->size());
//        // TODO: Require re-initialization
//    }
//}

} // namespace metal
