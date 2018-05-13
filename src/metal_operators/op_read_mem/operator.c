#include "operator.h"

#include <stdlib.h>
#include <stddef.h>
#include <endian.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

#include "../../metal_fpga/include/action_metalfpga.h"

#include "../../metal/metal.h"

static uint64_t _buffer_address = 0;
static uint64_t _buffer_length = 0;

static const void* handle_opts(mtl_operator_invocation_args *args, uint64_t *length, bool *valid) {
    *length = 0;
    *valid = true;
    return "";
}

static void print_memory_64(uint8_t * mem)
{
    for (int i = 0; i < 64; ++i) {
        printf("%02x ", ((uint8_t*)mem)[i]);
        if (i%8 == 7) printf("\n");
    }
}

static int apply_config(struct snap_action *action) {
    uint64_t *job_struct = (uint64_t*)snap_malloc(2 * sizeof(uint64_t));
    job_struct[0] = htobe64(_buffer_address);
    job_struct[1] = htobe64(_buffer_length);

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_AFU_MEM_SET_READ_BUFFER;
    mjob.job_address = (uint64_t)job_struct;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(action, &cjob, timeout);

    free(job_struct);

    // printf("Input memory is set to\n");
    // print_memory_64((uint8_t*)_buffer_address);

    if (rc != 0)
        // Some error occurred
        return MTL_ERROR_INVALID_ARGUMENT;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return MTL_ERROR_INVALID_ARGUMENT;

    return MTL_SUCCESS;
}

mtl_operator_specification op_read_mem_specification = {
    { OP_READ_MEM_ENABLE_ID, OP_MEM_STREAM_ID },
    "read_mem",
    true,

    &handle_opts,
    &apply_config,
    NULL,
    NULL
};

void op_read_mem_set_buffer(void *buffer, uint64_t length) {
    _buffer_address = (uint64_t)buffer;
    _buffer_length = length;
}
