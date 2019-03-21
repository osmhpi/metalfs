#include <utility>


extern "C" {
#include <metal_filesystem/metal.h>
#include <unistd.h>
#include <snap_hls_if.h>
}

#include <metal_pipeline/snap_action.hpp>
#include <metal_fpga/hw/hls/include/action_metalfpga.h>
#include "file_data_sink.hpp"

namespace metal {

FileDataSink::FileDataSink(std::string filename, uint64_t offset, uint64_t size)
    : CardMemoryDataSink(0, size), _offset(offset), _filename(std::move(filename)) {
  if (size > 0) {
    prepareForTotalProcessingSize(offset + size);
  } else {
    // User must call setSize later, extents remain uninitialized for now.
  }
}

FileDataSink::FileDataSink(std::vector<mtl_file_extent> &extents, uint64_t offset, uint64_t size)
        : CardMemoryDataSink(0, size), _extents(extents), _offset(offset) {}

void FileDataSink::prepareForTotalProcessingSize(size_t size) {

  if (_filename.empty())
    return;

  if (size == _cached_total_size)
    return;

  int res;

  uint64_t inode_id;
  res = mtl_open(_filename.c_str(), &inode_id);

  if (res != MTL_SUCCESS)
    throw std::runtime_error("File does not exist.");

  res = mtl_truncate(inode_id, size);
  if (res != MTL_SUCCESS)
    throw std::runtime_error("Unable to update file length");

  loadExtents();
}

#define WRITE_FILE_DRAM_BASEADDR 0x80000000

void FileDataSink::configure(SnapAction &action) {

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
    job_struct[0] = htobe64(1);  // slot number
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

  auto file_contents_in_dram_offset = WRITE_FILE_DRAM_BASEADDR + (_offset % NVME_BLOCK_BYTES);

  // If we only overwrite half of the first touched block load it to DRAM first
  if (_offset % NVME_BLOCK_BYTES) {
    auto *job_struct = (uint64_t*)snap_malloc(4 * sizeof(uint64_t));
    job_struct[0] = htobe64(1); // Slot
    job_struct[1] = htobe64(_offset % NVME_BLOCK_BYTES); // File offset
    job_struct[2] = htobe64(WRITE_FILE_DRAM_BASEADDR); // DRAM target address
    job_struct[3] = htobe64(1); // number of blocks to load

    try {
      action.execute_job(MTL_JOB_MOUNT, reinterpret_cast<char *>(job_struct));
    } catch (std::exception &ex) {
      free(job_struct);
      throw ex;
    }

    free(job_struct);
  }

  // If we only overwrite half of the last touched block load it to DRAM first
  if ((_offset + _size) % NVME_BLOCK_BYTES) {
    auto *job_struct = (uint64_t*)snap_malloc(4 * sizeof(uint64_t));
    job_struct[0] = htobe64(1); // Slot
    job_struct[1] = htobe64(((_offset + _size) / NVME_BLOCK_BYTES) * NVME_BLOCK_BYTES); // File offset
    job_struct[2] = htobe64(((file_contents_in_dram_offset + _size) / NVME_BLOCK_BYTES) * NVME_BLOCK_BYTES); // DRAM target address
    job_struct[3] = htobe64(1); // number of blocks to load

    try {
      action.execute_job(MTL_JOB_MOUNT, reinterpret_cast<char *>(job_struct));
    } catch (std::exception &ex) {
      free(job_struct);
      throw ex;
    }

    free(job_struct);
  }

  setAddress(file_contents_in_dram_offset);

  CardMemoryDataSink::configure(action);
}

void FileDataSink::finalize(SnapAction &action) {
  CardMemoryDataSink::finalize(action);

  // Advance offset
  _offset += _size;
}

void FileDataSink::setSize(size_t size) {
  CardMemoryDataSink::setSize(size);

  // Make sure that the file is big enough
  if (size + _offset > _cached_total_size) {
    prepareForTotalProcessingSize(size + _offset);
  }
}

void FileDataSink::loadExtents() {
  std::vector<mtl_file_extent> extents(MTL_MAX_EXTENTS);
  uint64_t extents_length, file_length;
  if (mtl_load_extent_list(_filename.c_str(), extents.data(), &extents_length, &file_length) != MTL_SUCCESS)
    throw std::runtime_error("Unable to load extents");

  extents.resize(extents_length);

  _extents = extents;
  _cached_total_size = file_length;
}

} // namespace metal
