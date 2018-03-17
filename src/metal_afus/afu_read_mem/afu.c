#include "afu.h"

#include <stddef.h>
#include <endian.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <action_metalfpga.h>
#include <snap_hls_if.h>

#include "action_metalfpga.h"
#include "../../metal/metal.h"

#define AFU_READ_MEM_ID 0

uint64_t _buffer_address = 0;
uint64_t _buffer_length = 0;

static const void* handle_opts(int argc, char *argv[], uint64_t *length, bool *valid) {
    *length = 0;
    *valid = true;
    return "";
}

static int apply_config(struct snap_action *action, struct snap_job *cjob, metalfpga_job_t *mjob) {
    const unsigned long timeout = 600;

    uint64_t *job_struct = (uint64_t*)snap_malloc(2 * sizeof(uint64_t));
    job_struct[0] = htobe64(_buffer_address);
    job_struct[1] = htobe64(_buffer_length);

    memset(mjob, 0, sizeof(*mjob));
    mjob->job_type = MF_JOB_AFU_MEM_SET_READ_BUFFER;
    mjob->job_address = (uint64_t)job_struct;
    snap_job_set(cjob, mjob, sizeof(*mjob), NULL, 0);

    int rc = snap_action_sync_execute_job(action, &cjob, timeout);

    free(job_struct);

    if (rc != 0)
        // Some error occurred
        return MTL_ERROR_INVALID_ARGUMENT;

    if (cjob->retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return MTL_ERROR_INVALID_ARGUMENT;

    return MTL_SUCCESS;
}

mtl_afu_specification afu_read_mem_specification = {
    AFU_READ_MEM_ID,
    "read_mem",

    &handle_opts,
    &apply_config
};

void afu_read_mem_set_buffer(void *buffer, uint64_t length) {

}
