#include <unistd.h>

#include <utility>

#include <spdlog/spdlog.h>

#include <metal-filesystem/metal.h>
#include <snap_action_metal.h>
#include <metal-filesystem-pipeline/file_data_source_context.hpp>
#include <metal-pipeline/snap_action.hpp>

namespace metal {

size_t FileDataSourceContext::reportTotalSize() { return loadExtents(); }

FileDataSourceContext::FileDataSourceContext(fpga::AddressType resource,
                                                   fpga::MapType map,
                                                   std::string filename,
                                                   uint64_t offset,
                                                   uint64_t size)
    : DefaultDataSourceContext(DataSource(offset, size, resource, map)),
      _filename(std::move(filename)) {
  if (size > 0) {
    if (_filename.empty()) return;
    loadExtents();
  } else {
    // User must call setSize later, extents remain uninitialized for now.
  }
}

FileDataSourceContext::FileDataSourceContext(
    fpga::AddressType resource, fpga::MapType map,
    std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t size)
    : DefaultDataSourceContext(DataSource(offset, size, resource, map)),
      _extents(extents) {}

uint64_t FileDataSourceContext::loadExtents() {
  std::vector<mtl_file_extent> extents(MTL_MAX_EXTENTS);
  uint64_t extents_length, file_length;
  if (mtl_load_extent_list(_filename.c_str(), extents.data(), &extents_length,
                           &file_length) != MTL_SUCCESS)
    throw std::runtime_error("Unable to load extents");

  extents.resize(extents_length);

  _extents = std::move(extents);
  _file_length = file_length;
  return file_length;
}

void FileDataSourceContext::configure(SnapAction &action, bool) {
  if (_extents.empty())
    throw std::runtime_error("Extents were not initialized");

  // TODO: There is a potential race condition when the file metadata was
  // modified between obtaining the extent list and calling configure()

  // Transfer extent list

  auto *job_struct = reinterpret_cast<uint64_t *>(action.allocateMemory(
      sizeof(uint64_t) * (8                        // words for the prefix
                          + (2 * _extents.size())  // two words for each extent
                          )));
  job_struct[0] = htobe64(
      static_cast<uint64_t>(fpga::ExtmapSlot::NVMeRead));  // slot number
  job_struct[1] = htobe64(1);                              // map (vs unmap)
  job_struct[2] = htobe64(_extents.size());                // extent count
  spdlog::trace("Mapping {} extents for reading", _extents.size());

  for (uint64_t i = 0; i < _extents.size(); ++i) {
    job_struct[8 + 2 * i + 0] = htobe64(_extents[i].offset);
    job_struct[8 + 2 * i + 1] = htobe64(_extents[i].length);
    spdlog::trace("  Offset {}  Length {}", _extents[i].offset,
                  _extents[i].length);
  }

  try {
    action.execute_job(fpga::JobType::Map,
                       reinterpret_cast<char *>(job_struct));
  } catch (std::exception &ex) {
    free(job_struct);
    throw ex;
  }

  free(job_struct);
}

void FileDataSourceContext::finalize(SnapAction &action) {
  (void)action;
  // Advance offset
  _dataSource =
      DataSource(_dataSource.address().addr + _dataSource.address().size,
                 _dataSource.address().size, _dataSource.address().type,
                 _dataSource.address().map);
}

bool FileDataSourceContext::endOfInput() const {
  return _dataSource.address().addr + _dataSource.address().size >=
         _file_length;
}

}  // namespace metal
