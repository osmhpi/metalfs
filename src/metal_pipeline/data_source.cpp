extern "C" {
#include <unistd.h>
#include <snap_hls_if.h>
}

#include <metal_fpga/hw/hls/include/action_metalfpga.h>
#include <iostream>
#include "data_source.hpp"
#include "snap_action.hpp"

namespace metal {


void HostMemoryDataSource::configure(SnapAction &action) {
    auto *job_struct = (uint64_t*)snap_malloc(3 * sizeof(uint64_t));
    job_struct[0] = htobe64(reinterpret_cast<uint64_t>(_dest));
    job_struct[1] = htobe64(_size);
    job_struct[2] = htobe64(OP_MEM_MODE_HOST);

    try {
        action.execute_job(MTL_JOB_OP_MEM_SET_READ_BUFFER, reinterpret_cast<char *>(job_struct));
    } catch (std::exception &ex) {
        free(job_struct);
        throw ex;
    }

    free(job_struct);
}

void CardMemoryDataSource::configure(SnapAction &action) {
    (void)action;
    // TODO
}

void RandomDataSource::configure(SnapAction &action) {
    auto *job_struct = (uint64_t*)snap_malloc(3 * sizeof(uint64_t));
    job_struct[0] = htobe64(0);
    job_struct[1] = htobe64(_size);
    job_struct[2] = htobe64(OP_MEM_MODE_RANDOM);

    try {
        action.execute_job(MTL_JOB_OP_MEM_SET_READ_BUFFER, reinterpret_cast<char *>(job_struct));
    } catch (std::exception &ex) {
        free(job_struct);
        throw ex;
    }

    free(job_struct);
}

void DataSource::setSize(size_t size) {
  _size = size;
}

} // namespace metal
