#include <utility>


extern "C" {
#include <metal_filesystem/metal.h>
#include <unistd.h>
#include <snap_hls_if.h>
}

#include <metal_pipeline/snap_action.hpp>
#include <metal_fpga/hw/hls/include/action_metalfpga.h>
#include "file_data_source.hpp"

namespace metal {

//uint64_t metal::FileDataSource::file_size() const {
//
//    // Resolve extent list
//    mtl_prepare_storage_for_reading(_filename.c_str(), nullptr);
//    return 0;
//}

FileDataSource::FileDataSource(std::string filename, uint64_t offset, uint64_t size)
        : CardMemoryDataSource(0, size), _offset(offset), _filename(std::move(filename)) {
  if (size > 0) {
    if (_filename.empty())
      return;
    loadExtents();
  } else {
    // User must call setSize later, extents remain uninitialized for now.
  }
}

FileDataSource::FileDataSource(std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t size)
        : CardMemoryDataSource(0, size), _extents(extents), _offset(offset) {

}

void FileDataSource::loadExtents() {
  std::vector<mtl_file_extent> extents(MTL_MAX_EXTENTS);
  uint64_t extents_length, file_length;
  if (mtl_load_extent_list(_filename.c_str(), extents.data(), &extents_length, &file_length) != MTL_SUCCESS)
    throw std::runtime_error("Unable to load extents");

  extents.resize(extents_length);

  _extents = extents;
//  _cached_total_size = file_length;
}

#define READ_FILE_DRAM_BASEADDR 0x00000000

void FileDataSource::configure(SnapAction &action) {

  if (_extents.empty())
    throw std::runtime_error("Extents were not initialized");

  // TODO: There is a potential race condition when the file metadata was modified between obtaining the extent list and calling configure()

  // Transfer extent list
  {
    auto *job_struct = (uint64_t*)snap_malloc(
            sizeof(uint64_t) * (
                    8 // words for the prefix
                    + (2 * _extents.size()) // two words for each extent
            )
    );
    job_struct[0] = htobe64(0);  // slot number
    job_struct[1] = htobe64(1);  // map (vs unmap)
    job_struct[2] = htobe64(_extents.size());  // extent count

    for (uint64_t i = 0; i < _extents.size(); ++i) {
      job_struct[8 + 2*i + 0] = htobe64(_extents[i].offset);
      job_struct[8 + 2*i + 1] = htobe64(_extents[i].length);
    }

    try {
      action.execute_job(MTL_JOB_MAP, reinterpret_cast<char *>(job_struct));
    } catch (std::exception &ex) {
      free(job_struct);
      throw ex;
    }

    free(job_struct);
  }

  auto file_contents_in_dram_offset = READ_FILE_DRAM_BASEADDR + (_offset % NVME_BLOCK_BYTES);

  uint64_t block_offset = _offset / NVME_BLOCK_BYTES;
  uint64_t end_block_offset = (_offset + _size) / NVME_BLOCK_BYTES;
  uint64_t blocks_length = end_block_offset - block_offset + 1;

  auto *job_struct = (uint64_t*)snap_malloc(4 * sizeof(uint64_t));
  job_struct[0] = htobe64(0); // Slot
  job_struct[1] = htobe64(_offset % NVME_BLOCK_BYTES); // File offset
  job_struct[2] = htobe64(READ_FILE_DRAM_BASEADDR); // DRAM target address
  job_struct[3] = htobe64(blocks_length); // number of blocks to load

  try {
    action.execute_job(MTL_JOB_MOUNT, reinterpret_cast<char *>(job_struct));
  } catch (std::exception &ex) {
    free(job_struct);
    throw ex;
  }

  free(job_struct);

  setAddress(file_contents_in_dram_offset);

  CardMemoryDataSource::configure(action);
}

void FileDataSource::finalize(SnapAction &action) {
  CardMemoryDataSource::finalize(action);

  // Advance offset
  _offset += _size;
}

} // namespace metal
