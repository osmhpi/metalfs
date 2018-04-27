#include "operator.h"

#include <stdlib.h>
#include <stddef.h>
#include <endian.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

#include "../../metal_fpga/include/action_metalfpga.h"

#include "../../metal/metal.h"

#define AFU_WRITE_FILE_ID 2

static uint64_t *_job_config = NULL;

static const void* handle_opts(mtl_operator_invocation_args *args, uint64_t *length, bool *valid) {
    *length = 0;
    *valid = true;
    return "";
}

static const char* get_filename() {
    return NULL;
}

static int apply_config(struct snap_action *action) {
    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_MAP;
    mjob.job_address = (uint64_t)_job_config;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(action, &cjob, timeout);

    if (rc != 0)
        // Some error occurred
        return MTL_ERROR_INVALID_ARGUMENT;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return MTL_ERROR_INVALID_ARGUMENT;

    return MTL_SUCCESS;
}

mtl_operator_specification op_write_file_specification = {
    { AFU_WRITE_FILE_ID, 1 },
    "write_file",
    false,

    &handle_opts,
    &apply_config,
    &get_filename
};

void op_write_file_set_buffer(uint64_t offset, uint64_t length) {

}

void op_write_file_set_extents(const mtl_file_extent *extents, uint64_t length) {
    if (_job_config) {
        free(_job_config);
        _job_config = NULL;
    }

    // Allocate job struct memory aligned on a page boundary
    uint64_t *_job_config = (uint64_t*) snap_malloc(
        sizeof(uint64_t) * (
            8 +          // words for the prefix
            (2 * length) // two words for each extent
        )
    );

    _job_config[0] = 1;       // slot number
    _job_config[1] = true;    // map (vs unmap)
    _job_config[2] = length;  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        _job_config[8 + 2*i + 0] = extents[i].offset;
        _job_config[8 + 2*i + 1] = extents[i].length;
    }
}
