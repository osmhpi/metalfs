#include "storage.h"

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../metal/metal.h"

#ifdef WITH_SNAP

#include <libsnap.h>
#include <snap_tools.h>
#include <snap_s_regs.h>

#include "action_metalfpga.h"

#define ACTION_WAIT_TIME        1                /* Default timeout in sec */

#define KILO_BYTE               (1024ull)
#define MEGA_BYTE               (1024 * KILO_BYTE)
#define GIGA_BYTE               (1024 * MEGA_BYTE)
#define DDR_MEM_SIZE            (4 * GIGA_BYTE)   /* Default End of FPGA Ram */
#define DDR_MEM_BASE_ADDR       0x00000000        /* Default Start of FPGA Ram */
#define HOST_BUFFER_SIZE        (256 * KILO_BYTE) /* Default Size for Host Buffers */
#define NVME_LB_SIZE            512               /* NVME Block Size */
#define NVME_DRIVE_SIZE         (4 * GIGA_BYTE)	  /* NVME Drive Size */
#define NVME_MAX_TRANSFER_SIZE  (32 * MEGA_BYTE) /* NVME limit to Transfer in one chunk */

static void *get_mem(int size)
{
    void *buffer;

    if (posix_memalign((void **)&buffer, 4096, size) != 0) {
        perror("FAILED: posix_memalign");
        return NULL;
    }
    return buffer;
}

static void free_mem(void *buffer)
{
    if (buffer)
        free(buffer);
}

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
        metadata->num_blocks = NVME_DRIVE_SIZE / NVME_LB_SIZE;
        metadata->block_size = NVME_LB_SIZE;
    }

    return MTL_SUCCESS;
}

mtl_file_extent *_extents = NULL;
uint64_t _extents_length;
int mtl_storage_set_active_extent_list(const mtl_file_extent *extents, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)get_mem(
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

    free_mem(job_words);

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
    uint64_t *job_words = (uint64_t*)get_mem(sizeof(uint64_t) * 4);

    uint8_t *job_bytes = (uint8_t*) job_words;
    job_bytes[0] = 0;  // slot
    job_bytes[1] = true;  // write, don't read

    // Align the input buffer
    uint64_t aligned_length = length % 64 == 0 ? length : (length / 64 + 1) * 64;
    void *aligned_buffer = get_mem(aligned_length);
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

    free_mem(aligned_buffer);
    free_mem(job_words);

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
    uint64_t *job_words = (uint64_t*)get_mem(sizeof(uint64_t) * 4);

    uint8_t *job_bytes = (uint8_t*) job_words;
    job_bytes[0] = 0;  // slot
    job_bytes[1] = false;  // read, don't write

    // Align the temporary output buffer
    uint64_t aligned_length = length % 64 == 0 ? length : (length / 64 + 1) * 64;
    void *aligned_buffer = get_mem(aligned_length);

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

    free_mem(job_words);

    if (rc != 0) {
        free_mem(aligned_buffer);
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        free_mem(aligned_buffer);
        return MTL_ERROR_INVALID_ARGUMENT;
    }

    memcpy(buffer, aligned_buffer, length);

    free_mem(aligned_buffer);

    return MTL_SUCCESS;
}

#else

#define NUM_BLOCKS 128
#define BLOCK_SIZE 512

void *_storage = NULL;

mtl_file_extent *_extents = NULL;
uint64_t _extents_length;

int mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
    if (metadata) {
        metadata->num_blocks = NUM_BLOCKS;
        metadata->block_size = BLOCK_SIZE;
    }

    return 0;
}

int mtl_storage_set_active_extent_list(const mtl_file_extent *extents, uint64_t length) {
    free(_extents);

    _extents = malloc(length * sizeof(mtl_file_extent));
    memcpy(_extents, extents, length * sizeof(mtl_file_extent));
    _extents_length = length;
}

int mtl_storage_write(uint64_t offset, void *buffer, uint64_t length) {
    if (!_storage) {
        _storage = malloc(NUM_BLOCKS * BLOCK_SIZE);
    }

    assert(offset + length < NUM_BLOCKS * BLOCK_SIZE);

    uint64_t current_offset = offset;

    uint64_t current_extent = 0;
    uint64_t current_extent_offset = 0;
    while (length > 0) {
        // Determine the extent for the next byte to be written
        while (current_extent_offset + (_extents[current_extent].length * BLOCK_SIZE) <= current_offset) {
            current_extent_offset += _extents[current_extent].length * BLOCK_SIZE;
            ++current_extent;

            // The caller has to make sure the file is large enough to write
            assert(current_extent < _extents_length);
        }

        uint64_t current_extent_write_pos = current_offset - current_extent_offset;
        uint64_t current_extent_write_length =
            (_extents[current_extent].length * BLOCK_SIZE) - current_extent_write_pos;
        if (current_extent_write_length > length)
            current_extent_write_length = length;

        memcpy(_storage + (_extents[current_extent].offset * BLOCK_SIZE) + current_extent_write_pos,
               buffer + (current_offset - offset),
               current_extent_write_length);

        current_offset += current_extent_write_length;
        length -= current_extent_write_length;
    }

    return 0;
}

int mtl_storage_read(uint64_t offset, void *buffer, uint64_t length) {
    if (!_storage) {
        _storage = malloc(NUM_BLOCKS * BLOCK_SIZE);
    }

    assert(offset + length < NUM_BLOCKS * BLOCK_SIZE);

    uint64_t current_offset = offset;

    uint64_t current_extent = 0;
    uint64_t current_extent_offset = 0;
    while (length > 0) {
        // Determine the extent for the next byte to be read
        while (current_extent_offset + (_extents[current_extent].length * BLOCK_SIZE) <= current_offset) {
            current_extent_offset += _extents[current_extent].length * BLOCK_SIZE;
            ++current_extent;

            // The caller has to make sure the file is large enough to read
            assert(current_extent < _extents_length);
        }

        uint64_t current_extent_read_pos = current_offset - current_extent_offset;
        uint64_t current_extent_read_length =
            (_extents[current_extent].length * BLOCK_SIZE) - current_extent_read_pos;
        if (current_extent_read_length > length)
            current_extent_read_length = length;

        memcpy(buffer + (current_offset - offset),
               _storage + (_extents[current_extent].offset * BLOCK_SIZE) + current_extent_read_pos,
               current_extent_read_length);

        current_offset += current_extent_read_length;
        length -= current_extent_read_length;
    }

    return 0;
}

#endif
