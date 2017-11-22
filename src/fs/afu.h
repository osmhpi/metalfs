#pragma once

#include <stddef.h>

#include "list/list.h"

extern LIST_ENTRY fpga_buffers;

int create_new_buffer();

void perform_afu_action(
    int afu_type,
    int input_buf_handle,  // A buffer identifier on the FPGA
    char* input_buffer,  // A host-provided buffer
    size_t input_size,
    int output_buf_handle, // A buffer identifier on the FPGA
    char* output_buffer, // A host-provided buffer
    size_t* output_size
    );
