extern "C" {
#include <unistd.h>
#include <snap_hls_if.h>
}

#include <metal_fpga/hw/hls/include/snap_action_metal.h>
#include <iostream>
#include "data_source.hpp"
#include "snap_action.hpp"

namespace metal {


void HostMemoryDataSource::configure(SnapAction &action) {
    auto *job_struct = reinterpret_cast<uint64_t*>(snap_malloc(3 * sizeof(uint64_t)));
    // TODO: This is precisely snap_addr_t, why re-implement it?
    job_struct[0] = htobe64(reinterpret_cast<uint64_t>(_dest));
    job_struct[1] = htobe64(_size);
    job_struct[2] = htobe64(OP_MEM_MODE_HOST);

    try {
        action.execute_job(fpga::JobType::SetReadBuffer, reinterpret_cast<char *>(job_struct));
    } catch (std::exception &ex) {
        free(job_struct);
        throw ex;
    }

    free(job_struct);
}

void CardMemoryDataSource::configure(SnapAction &action) {
    auto *job_struct = reinterpret_cast<uint64_t*>(snap_malloc(3 * sizeof(uint64_t)));
    job_struct[0] = htobe64(_address);
    job_struct[1] = htobe64(_size);
    job_struct[2] = htobe64(OP_MEM_MODE_DRAM);

    try {
        action.execute_job(fpga::JobType::SetReadBuffer, reinterpret_cast<char *>(job_struct));
    } catch (std::exception &ex) {
        free(job_struct);
        throw ex;
    }

    free(job_struct);
}

RandomDataSource::RandomDataSource(size_t size) : DataSource(size) {
    _optionDefinitions.insert(std::make_pair("length", OperatorOptionDefinition(
        0, OptionType::INT, "length", "l", "The amount of data to generate, in bytes (default: 4096)"
    )));

    _options.insert(std::make_pair("length", 4096));
}

size_t RandomDataSource::reportTotalSize() {
    // TODO: Does this belong here? Maybe find a better method name then...
    _size = std::get<int>(_options.at("length").value());
    return _size;
}

void RandomDataSource::configure(SnapAction &action) {
    auto *job_struct = reinterpret_cast<uint64_t*>(snap_malloc(3 * sizeof(uint64_t)));
    job_struct[0] = htobe64(0);
    job_struct[1] = htobe64(_size);
    job_struct[2] = htobe64(OP_MEM_MODE_RANDOM);

    try {
        action.execute_job(fpga::JobType::SetReadBuffer, reinterpret_cast<char *>(job_struct));
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
