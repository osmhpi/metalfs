#include "storage.h"

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>

#ifdef WITH_SNAP

#include <libsnap.h>
#include <snap_tools.h>
#include <snap_s_regs.h>

#include "snap_example.h"

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

/* Action or Kernel Write and Read are 32 bit MMIO */
static void action_write(struct snap_card* h, uint32_t addr, uint32_t data)
{
    int rc;

    rc = snap_mmio_write32(h, (uint64_t)addr, data);
    return;
}

static int action_wait_idle(struct snap_card* h, int timeout, uint32_t mem_size)
{
    int rc = ETIME;
    uint64_t t_start;	/* time in usec */
    uint64_t td;		/* Diff time in usec */

    /* FIXME Use act and not h */
    snap_action_start((void*)h);

    /* FIXME Use act and not h */
    rc = snap_action_completed((void*)h, NULL, timeout);

    return(!rc);
}

static void action_memcpy(struct snap_card* h,
        uint32_t action,
        uint64_t dest,
        uint64_t src,
        size_t n)
{
    action_write(h, ACTION_CONFIG,  action);
    action_write(h, ACTION_DEST_LOW, (uint32_t)(dest & 0xffffffff));
    action_write(h, ACTION_DEST_HIGH, (uint32_t)(dest >> 32));
    action_write(h, ACTION_SRC_LOW, (uint32_t)(src & 0xffffffff));
    action_write(h, ACTION_SRC_HIGH, (uint32_t)(src >> 32));
    action_write(h, ACTION_CNT, n);
    return;
}


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

void fpga_copy(uint32_t blocks, uint64_t nvme_block_addr, void *buf, bool write) {
    int card_no = 0;
    char device[64];
    struct snap_card *dn;	/* lib snap handle */
    struct snap_action *act = NULL;
    unsigned long have_nvme = 0;
    int timeout = ACTION_WAIT_TIME;
    uint64_t ddr_addr = 0;
    uint64_t host_addr = 0;
    snap_action_flag_t attach_flags = 0;
    int drive = 0;
    uint32_t drive_cmd = ACTION_CONFIG_COPY_HD;

    if (((nvme_block_addr + blocks) * NVME_LB_SIZE) > NVME_DRIVE_SIZE) {
        return;
    }

    sprintf(device, "/dev/cxl/afu%d.0s", card_no);
    dn = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);

    /* Check if i do have NVME */
    snap_card_ioctl(dn, GET_NVME_ENABLED, (unsigned long)&have_nvme);
    if (0 == have_nvme) {
        // Interesting...
    }

    act = snap_attach_action(dn, ACTION_TYPE_EXAMPLE, attach_flags, 5*timeout);
    if (NULL == act) {
        // Yes, we *could* handle this...
    }

    host_addr = (uint64_t)buf;

    if (write) {
        // Copy from host to DDR
        action_memcpy(dn, ACTION_CONFIG_COPY_HD, ddr_addr, host_addr, blocks * NVME_LB_SIZE);
        action_wait_idle(dn, timeout, blocks * NVME_LB_SIZE);

        // Copy from DDR to NVME
        drive_cmd = ACTION_CONFIG_COPY_DN | (NVME_DRIVE1 * drive);
        action_memcpy(dn, drive_cmd, nvme_block_addr, ddr_addr, blocks);
        action_wait_idle(dn, timeout, blocks * NVME_LB_SIZE);
    } else {
        // Copy from NVME to DDR
        drive_cmd = ACTION_CONFIG_COPY_ND | (NVME_DRIVE1 * drive);
        action_memcpy(dn, drive_cmd, ddr_addr, nvme_block_addr, blocks);
        action_wait_idle(dn, timeout, blocks * NVME_LB_SIZE);

        // Copy from DDR to host
        action_memcpy(dn, ACTION_CONFIG_COPY_DH, host_addr, ddr_addr, blocks * NVME_LB_SIZE);
        action_wait_idle(dn, timeout, blocks * NVME_LB_SIZE);
    }

    snap_detach_action(act);
    snap_card_free(dn);
}

int mtl_storage_get_metadata(mtl_storage_metadata *metadata) {
    if (metadata) {
        metadata->num_blocks = NVME_DRIVE_SIZE / NVME_LB_SIZE;
        metadata->block_size = NVME_LB_SIZE;
    }

    return 0;
}

mtl_file_extent *_extents = NULL;
uint64_t _extents_length;
int mtl_storage_set_active_extent_list(const mtl_file_extent *extents, uint64_t length) {
    free(_extents);

    _extents = malloc(length * sizeof(mtl_file_extent));
    memcpy(_extents, extents, length * sizeof(mtl_file_extent));
    _extents_length = length;

    return 0;
}

int mtl_storage_write(uint64_t offset, void *buffer, uint64_t length) {
    uint64_t current_offset = offset;   // Our current writing position inside the file

    uint64_t current_extent = 0;
    uint64_t current_extent_offset = 0; // The current extent's offset inside the file

    // Process data from one extent in each loop iteration
    while (length > 0) {

        // Determine the extent for the next byte to be written
        while (current_extent_offset + (_extents[current_extent].length * NVME_LB_SIZE) <= current_offset) {
            current_extent_offset += _extents[current_extent].length * NVME_LB_SIZE;
            ++current_extent;

            // The caller has to make sure the file is large enough to write
            assert(current_extent < _extents_length);
        }

        // We have found the next extent to write to
        // Now calculate the offset inside the extent where we want to write
        uint64_t current_extent_write_pos = current_offset - current_extent_offset;
        // The corresponding block starts at or before this position (so integer division is fine)
        uint64_t current_extent_write_pos_block = current_extent_write_pos / NVME_LB_SIZE;

        // Determine how many bytes we want to write
        uint64_t current_extent_write_length =
            (_extents[current_extent].length * NVME_LB_SIZE) - current_extent_write_pos;
        if (current_extent_write_length > length)
            current_extent_write_length = length;

        // Because we can only transfer full blocks, we have to copy the data to a
        // memory-aligned temporary buffer. We also copy in batches of at most 4KiB
        // (although we could theoretically copy up to NVME_MAX_TRANSFER_SIZE bytes)
        uint64_t temp_block_buffer_size = 4 * KILO_BYTE;
        if (current_extent_write_length < 4 * KILO_BYTE) {
            temp_block_buffer_size = current_extent_write_length / NVME_LB_SIZE;
            if (current_extent_write_length % NVME_LB_SIZE)
                ++temp_block_buffer_size;
            temp_block_buffer_size *= NVME_LB_SIZE;
        }
        void *temp_block_buffer = get_mem(temp_block_buffer_size);

        // Call fpga_copy until there's no data left for the current extent
        while (current_extent_write_length > 0) {

            // Because we can only write block-aligned, we will clear the data
            // in the beginning of a block when we actually intended to start
            // writing in the middle of the block.
            // If this is the case, spend the first transfer to block-align our
            // writing position
            uint64_t temp_block_buffer_offset =
                current_extent_write_pos - (current_extent_write_pos_block * NVME_LB_SIZE);

            // Determine the bytes written in this batch
            uint64_t write_length = current_extent_write_length;
            if (write_length > 4 * KILO_BYTE - temp_block_buffer_offset) {
                write_length = 4 * KILO_BYTE - temp_block_buffer_offset;
            }

            // Clear the buffer memory if we're not writing 4KiB
            if (current_extent_write_length < 4 * KILO_BYTE) {
                memset(temp_block_buffer, 0, temp_block_buffer_size);
                write_length = current_extent_write_length;
            }

            // Copy to buffer and then to FPGA
            memcpy(temp_block_buffer + temp_block_buffer_offset, buffer + (current_offset - offset), write_length);
            fpga_copy(temp_block_buffer_size / NVME_LB_SIZE,
                _extents[current_extent].offset + current_extent_write_pos_block,
                temp_block_buffer,
                true);

            // Update our position / length variables
            current_offset += write_length;
            current_extent_write_pos += write_length;
            current_extent_write_pos_block = current_extent_write_pos / NVME_LB_SIZE;

            current_extent_write_length -= write_length;
            length -= write_length;
        }

        free_mem(temp_block_buffer);
    }

    return 0;
}

int mtl_storage_read(uint64_t offset, void *buffer, uint64_t length) {
    uint64_t current_offset = offset; // Our current reading position inside the file

    uint64_t current_extent = 0;
    uint64_t current_extent_offset = 0; // The current extent's offset inside the file

    // Process data from one extent in each loop iteration
    while (length > 0) {

        // Determine the extent for the next byte to be read
        while (current_extent_offset + (_extents[current_extent].length * NVME_LB_SIZE) <= current_offset) {
            current_extent_offset += _extents[current_extent].length * NVME_LB_SIZE;
            ++current_extent;

            // The caller has to make sure the file is large enough to read
            assert(current_extent < _extents_length);
        }

        // We have found the next extent to read from
        // Now calculate the offset inside the extent where we want to read
        uint64_t current_extent_read_pos = current_offset - current_extent_offset;
        // The corresponding block starts at or before this position (so integer division is fine)
        uint64_t current_extent_read_pos_block = current_extent_read_pos / NVME_LB_SIZE;

        // Determine how many bytes we want to read
        uint64_t current_extent_read_length =
            (_extents[current_extent].length * NVME_LB_SIZE) - current_extent_read_pos;
        if (current_extent_read_length > length)
            current_extent_read_length = length;

        // Because we can only transfer full blocks, we have to copy the data from a
        // memory-aligned temporary buffer. We also copy in batches of at most 4KiB
        // (although we could theoretically copy up to NVME_MAX_TRANSFER_SIZE bytes)
        uint64_t temp_block_buffer_size = 4 * KILO_BYTE;
        if (current_extent_read_length < 4 * KILO_BYTE) {
            temp_block_buffer_size = current_extent_read_length / NVME_LB_SIZE;
            if (current_extent_read_length % NVME_LB_SIZE)
                ++temp_block_buffer_size;
            temp_block_buffer_size *= NVME_LB_SIZE;
        }
        void *temp_block_buffer = get_mem(temp_block_buffer_size);

        // Call fpga_copy until there's no data left for the current extent
        while (current_extent_read_length > 0) {

            // Account for the case when we want to start reading in the middle
            // of a block
            uint64_t temp_block_buffer_offset =
                current_extent_read_pos - (current_extent_read_pos_block * NVME_LB_SIZE);

            // Determine the bytes read in this batch
            uint64_t read_length = current_extent_read_length;
            if (read_length > 4 * KILO_BYTE - temp_block_buffer_offset) {
                read_length = 4 * KILO_BYTE - temp_block_buffer_offset;
            }

            // Copy to buffer and then to destination
            fpga_copy(temp_block_buffer_size / NVME_LB_SIZE,
                _extents[current_extent].offset + current_extent_read_pos_block,
                temp_block_buffer,
                false);
            memcpy(buffer + (current_offset - offset),
                temp_block_buffer + temp_block_buffer_offset,
                read_length);

            // Update our position / length variables
            current_offset += read_length;
            current_extent_read_pos += read_length;
            current_extent_read_pos_block = current_extent_read_pos / NVME_LB_SIZE;

            current_extent_read_length -= read_length;
            length -= read_length;
        }

        free_mem(temp_block_buffer);
    }

    return 0;
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
