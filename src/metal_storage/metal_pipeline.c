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
    int card_no = 0;
    char device[64];
    sprintf(device, "/dev/cxl/afu%d.0s", card_no);

    card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);

    if (card == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    /* Check if i do have NVME */
    unsigned long have_nvme = 0;
    snap_card_ioctl(card, GET_NVME_ENABLED, (unsigned long)&have_nvme);
    if (0 == have_nvme) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    int timeout = ACTION_WAIT_TIME;
    snap_action_flag_t attach_flags = 0;
    action = snap_attach_action(card, METALFPGA_ACTION_TYPE, attach_flags, 5 * timeout);
    if (NULL == action) {
        snap_card_free(card);
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    return MTL_SUCCESS;
}

int mtl_storage_deinitialize() {
    snap_detach_action(action);
    snap_card_free(card);

    return MTL_SUCCESS;
}

int mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
    if (metadata) {
        metadata->num_blocks = 64 * 1024 * 1024;
        metadata->block_size = 4096;
    }

    return MTL_SUCCESS;
}

int mtl_storage_set_active_read_extent_list(const mtl_file_extent *extents, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(
        sizeof(uint64_t) * (
            8 // words for the prefix
            + (2 * length) // two words for each extent
        )
    );

    job_words[0] = 0;  // slot number
    job_words[1] = true;  // map (vs unmap)
    job_words[2] = length;  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        job_words[8 + 2*i + 0] = extents[i].offset;
        job_words[8 + 2*i + 1] = extents[i].length;
    }

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_MAP;
    mjob.job_address = (uint64_t)job_words;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    int timeout = ACTION_WAIT_TIME;
    int rc = snap_action_sync_execute_job(action, &cjob, timeout);

    free(job_words);

    if (rc != 0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    return MTL_SUCCESS;
}

int mtl_storage_set_active_write_extent_list(const mtl_file_extent *extents, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(
        sizeof(uint64_t) * (
            8 // words for the prefix
            + (2 * length) // two words for each extent
        )
    );

    job_words[0] = 0;  // slot number
    job_words[1] = true;  // map (vs unmap)
    job_words[2] = length;  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        job_words[8 + 2*i + 0] = extents[i].offset;
        job_words[8 + 2*i + 1] = extents[i].length;
    }

    metalfpga_job_t mjob;
    mjob.job_type = MF_JOB_MAP;
    mjob.job_address = (uint64_t)job_words;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    int timeout = ACTION_WAIT_TIME;
    int rc = snap_action_sync_execute_job(action, &cjob, timeout);

    free(job_words);

    if (rc != 0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }

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
