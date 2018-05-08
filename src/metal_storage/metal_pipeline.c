#include "storage.h"

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../metal/metal.h"

#include <libsnap.h>
#include <snap_tools.h>
#include <snap_s_regs.h>
#include <snap_hls_if.h>

#include "../metal_fpga/include/action_metalfpga.h"

#define ACTION_WAIT_TIME        1                /* Default timeout in sec */

struct snap_card *card = NULL;
struct snap_action *action = NULL;

int mtl_storage_initialize() {

    return mtl_pipeline_initialize();
}

int mtl_storage_deinitialize() {

    return mtl_pipeline_deinitialize();
}

int mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
    if (metadata) {
        metadata->num_blocks = 64 * 1024 * 1024;
        metadata->block_size = 4096;
    }

    return MTL_SUCCESS;
}

int mtl_storage_set_active_read_extent_list(const mtl_file_extent *extents, uint64_t length) {

    mtl_pipeline_set_file_read_extent_list(extents, length);
    return MTL_SUCCESS;
}

int mtl_storage_set_active_write_extent_list(const mtl_file_extent *extents, uint64_t length) {

    mtl_pipeline_set_file_write_extent_list(extents, length);
    return MTL_SUCCESS;
}

int mtl_storage_write(uint64_t offset, void *buffer, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(sizeof(uint64_t) * 4);

    uint8_t *job_bytes = (uint8_t*) job_words;
    job_bytes[0] = 0;  // slot
    job_bytes[1] = true;  // write, don't read

    // Align the input buffer
    uint64_t aligned_length = length % 64 == 0 ? length : (length / 64 + 1) * 64;
    void *aligned_buffer = snap_malloc(aligned_length);
    memcpy(aligned_buffer, buffer, length);

    job_words[1] = (uint64_t) aligned_buffer;
    job_words[2] = offset;
    job_words[3] = length;

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_ACCESS;
    mjob.job_address = (uint64_t)job_words;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    int timeout = ACTION_WAIT_TIME;
    int rc = snap_action_sync_execute_job(action, &cjob, timeout);

    free(aligned_buffer);
    free(job_words);

    if (rc != 0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    return MTL_SUCCESS;
}

int mtl_storage_read(uint64_t offset, void *buffer, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(sizeof(uint64_t) * 4);

    uint8_t *job_bytes = (uint8_t*) job_words;
    job_bytes[0] = 0;  // slot
    job_bytes[1] = false;  // read, don't write

    // Align the temporary output buffer
    uint64_t aligned_length = length % 64 == 0 ? length : (length / 64 + 1) * 64;
    void *aligned_buffer = snap_malloc(aligned_length);

    job_words[1] = (uint64_t) aligned_buffer;
    job_words[2] = offset;
    job_words[3] = length;

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_ACCESS;
    mjob.job_address = (uint64_t)job_words;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    int timeout = ACTION_WAIT_TIME;
    int rc = snap_action_sync_execute_job(action, &cjob, timeout);

    free(job_words);

    if (rc != 0) {
        free(aligned_buffer);
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        free(aligned_buffer);
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    memcpy(buffer, aligned_buffer, length);

    free(aligned_buffer);

    return MTL_SUCCESS;
}
