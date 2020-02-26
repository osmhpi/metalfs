extern "C" {
#include <metal-filesystem/metal.h>
#include <unistd.h>
}

#include <spdlog/spdlog.h>
#include <utility>

#include <metal-pipeline/fpga_interface.hpp>
#include <metal-filesystem-pipeline/file_data_sink_context.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

FileDataSinkContext::FileDataSinkContext(fpga::AddressType resource,
                                               fpga::MapType map,
                                               std::string filename,
                                               uint64_t offset, uint64_t size)
    : DefaultDataSinkContext(DataSink(offset, size, resource, map)),
      _filename(std::move(filename)),
      _cachedTotalSize(0) {
  if (size > 0) {
    prepareForTotalSize(offset + size);
  } else {
    // User must call setSize later, extents remain uninitialized for now.
  }
}

FileDataSinkContext::FileDataSinkContext(
    fpga::AddressType resource, fpga::MapType map,
    std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t size)
    : DefaultDataSinkContext(DataSink(offset, size, resource, map)),
      _extents(extents),
      _cachedTotalSize(0) {}

void FileDataSinkContext::prepareForTotalSize(uint64_t size) {
  if (_filename.empty()) return;

  if (size == _cachedTotalSize) return;

  int res;

  uint64_t inode_id;
  res = mtl_open(_filename.c_str(), &inode_id);

  if (res != MTL_SUCCESS) throw std::runtime_error("File does not exist.");

  res = mtl_truncate(inode_id, size);
  if (res != MTL_SUCCESS)
    throw std::runtime_error("Unable to update file length");

  loadExtents();
}

void FileDataSinkContext::configure(SnapAction &action, uint64_t inputSize, bool) {
  _dataSink =
      DataSink(_dataSink.address().addr, inputSize,
               _dataSink.address().type, _dataSink.address().map);

  if (_dataSink.address().addr + _dataSink.address().size > _cachedTotalSize) {
    prepareForTotalSize(_dataSink.address().addr + _dataSink.address().size);
  }

  // TODO: There is a potential race condition when the file metadata was
  // modified between obtaining the extent list and calling configure()

  // Transfer extent list

  auto *job_struct = reinterpret_cast<uint64_t *>(action.allocateMemory(
      sizeof(uint64_t) *
      (8                                // words for the prefix
       + (2 * fpga::MaxExtentsPerFile)  // two words for each extent
       )));
  job_struct[0] = htobe64(
      static_cast<uint64_t>(fpga::ExtmapSlot::NVMeWrite));  // slot number
  spdlog::trace("Mapping {} extents for writing", _extents.size());

  for (uint64_t i = 0; i < fpga::MaxExtentsPerFile; ++i) {
    if (i < _extents.size()) {
      job_struct[8 + 2 * i + 0] = htobe64(_extents[i].offset);
      job_struct[8 + 2 * i + 1] = htobe64(_extents[i].length);
      spdlog::trace("  Offset {}  Length {}", _extents[i].offset,
                    _extents[i].length);
    } else {
      job_struct[8 + 2 * i + 0] = 0;
      job_struct[8 + 2 * i + 1] = 0;
    }
  }

  try {
    action.executeJob(fpga::JobType::Map,
                       reinterpret_cast<char *>(job_struct));
  } catch (std::exception &ex) {
    free(job_struct);
    throw ex;
  }

  free(job_struct);
}

void FileDataSinkContext::finalize(SnapAction &, uint64_t outputSize, bool) {
  // Advance offset
  _dataSink =
      DataSink(_dataSink.address().addr + outputSize, _dataSink.address().size,
               _dataSink.address().type, _dataSink.address().map);

   // if (endOfInput) {
   //     if (_dataSink.address().addr < _cachedTotalSize && _filename.size()) {
   //        int res;    
   //        uint64_t inode_id;
   //        res = mtl_open(_filename.c_str(), &inode_id);

   //        if (res != MTL_SUCCESS) throw std::runtime_error("File does not exist.");

   //        res = mtl_truncate(inode_id, _dataSink.address().addr);
   //        if (res != MTL_SUCCESS)
   //          throw std::runtime_error("Unable to update file length");
   //     }
   // }
}

void FileDataSinkContext::loadExtents() {
  std::vector<mtl_file_extent> extents(MTL_MAX_EXTENTS);
  uint64_t extents_length, file_length;
  if (mtl_load_extent_list(_filename.c_str(), extents.data(), &extents_length,
                           &file_length) != MTL_SUCCESS)
    throw std::runtime_error("Unable to load extents");

  extents.resize(extents_length);

  _extents = extents;
  _cachedTotalSize = file_length;
}

}  // namespace metal
