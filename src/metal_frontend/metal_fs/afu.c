#include "afu.h"

#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#ifdef WITH_SNAP
#include <libsnap.h>
#endif

#include "../common/buffer.h"
#include "../common/known_afus.h"

#ifdef WITH_SNAP
#include "actions/blowfish/action_blowfish.h"
#include "actions/blowfish/snap_blowfish.h"
#endif

// Emulate buffers on the FPGA
typedef struct fpga_buffer {
    int id;
    char buffer[BUFFER_SIZE];
    LIST_ENTRY Link;
} fpga_buffer_t;

LIST_ENTRY fpga_buffers;
pthread_mutex_t fpga_mutex = PTHREAD_MUTEX_INITIALIZER;

int new_buffer_handle_id = 0;
int create_new_buffer() {
    pthread_mutex_lock(&fpga_mutex);

    fpga_buffer_t *new_buffer = malloc(sizeof(fpga_buffer_t));
    new_buffer->id = new_buffer_handle_id++;

    InsertTailList(&fpga_buffers, &new_buffer->Link);

    pthread_mutex_unlock(&fpga_mutex);

    return new_buffer->id;
}

void release_buffer(int buffer_handle) {
    pthread_mutex_lock(&fpga_mutex);

    if (IsListEmpty(&fpga_buffers)) {
        pthread_mutex_unlock(&fpga_mutex);
        return;
    }

    PLIST_ENTRY current_link = fpga_buffers.Flink;
    fpga_buffer_t *current_buffer = NULL;
    do {
        current_buffer = CONTAINING_LIST_RECORD(current_link, fpga_buffer_t);

        if (current_buffer->id == buffer_handle)
            break;

        current_link = current_link->Flink;
    } while (current_link != &fpga_buffers);

    if (current_buffer) {
        RemoveEntryList(current_link);
        free(current_buffer);
    }

    pthread_mutex_unlock(&fpga_mutex);
}

char* get_buffer_addr(int buffer_handle) {
    pthread_mutex_lock(&fpga_mutex);

    if (IsListEmpty(&fpga_buffers)){
        pthread_mutex_unlock(&fpga_mutex);
        return NULL;
    }

    PLIST_ENTRY current_link = fpga_buffers.Flink;
    char* result = NULL;
    do {
        fpga_buffer_t *current_buffer = CONTAINING_LIST_RECORD(current_link, fpga_buffer_t);

        if (current_buffer->id == buffer_handle) {
            result = current_buffer->buffer;
            break;
        }

        current_link = current_link->Flink;
    } while (current_link != &fpga_buffers);

    pthread_mutex_unlock(&fpga_mutex);

    return result;
}

size_t perform_lowercase_action(int input_buf_handle, int output_buf_handle, size_t size) {
    // Inspired by snap_education

    char* input_buffer = get_buffer_addr(input_buf_handle);
    char* output_buffer = get_buffer_addr(output_buf_handle);

    // Convert all char to lower case
    for (size_t i = 0; i < size; ++i) {
        if (input_buffer[i] >= 'A' && input_buffer[i] <= 'Z')
            output_buffer[i] = input_buffer[i] + ('a' - 'A');
        else
            output_buffer[i] = input_buffer[i];
    }

    return size;
}

size_t perform_passthrough_action(int input_buf_handle, int output_buf_handle, size_t size) {
    char* input_buffer = get_buffer_addr(input_buf_handle);
    char* output_buffer = get_buffer_addr(output_buf_handle);

    // Just copy
    memcpy(output_buffer, input_buffer, size);
    return size;
}

static uint8_t example_key[] __attribute__((aligned(128))) = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};

#ifdef WITH_SNAP
size_t perform_blowfish_action(int input_buf_handle, int output_buf_handle, size_t size, bool encrypt) {
    char* input_buffer = get_buffer_addr(input_buf_handle);
    char* output_buffer = get_buffer_addr(output_buf_handle);

    unsigned long timeout = 10000;
    struct snap_card *card = NULL;
    struct snap_action *action = NULL;
    snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
    char device[128];

    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", 0);
    card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM,
                   SNAP_DEVICE_ID_SNAP);
    action = snap_attach_action(card, BLOWFISH_ACTION_TYPE, action_irq, 60);

    blowfish_set_key(action, timeout, example_key, sizeof(example_key));

    blowfish_cipher(action, encrypt ? MODE_ENCRYPT : MODE_DECRYPT, timeout,
        input_buffer, size,
        output_buffer, size);

    snap_detach_action(action);
    snap_card_free(card);

    return size;
}
#endif

void perform_afu_action(
    int afu_type,
    int input_buf_handle, char* input_buffer, size_t input_size,
    int output_buf_handle, char* output_buffer, size_t* output_size
) {
    // Do we have to copy from host to FPGA?
    if (input_buffer) {
        // Allocate a new buffer and remember its handle
        input_buf_handle = create_new_buffer();

        // Copy data from host to 'fpga'
        char* fpga_buffer = get_buffer_addr(input_buf_handle);
        memcpy(fpga_buffer, input_buffer, input_size);
    }

    // Do we have to copy from FPGA to host?
    if (output_buffer) {
        output_buf_handle = create_new_buffer();
    }

    switch (afu_type) {
//         case AFU_PASSTHROUGH:
//             *output_size = perform_passthrough_action(input_buf_handle, output_buf_handle, input_size);
//             break;
//         case AFU_UPPERCASE:
//             *output_size = perform_lowercase_action(input_buf_handle, output_buf_handle, input_size);
//             break;
// #ifdef WITH_SNAP
//         case AFU_BLOWFISH_ENCRYPT:
//             *output_size = perform_blowfish_action(input_buf_handle, output_buf_handle, input_size, true);
//             break;
//         case AFU_BLOWFISH_DECRYPT:
//             *output_size = perform_blowfish_action(input_buf_handle, output_buf_handle, input_size, false);
//             break;
// #endif
    }

    release_buffer(input_buf_handle);

    if (output_buffer) {
        char* fpga_buffer = get_buffer_addr(output_buf_handle);
        memcpy(output_buffer, fpga_buffer, *output_size);
        release_buffer(output_buf_handle);
    }
}
