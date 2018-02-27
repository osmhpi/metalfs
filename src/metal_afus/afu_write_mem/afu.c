#include "afu.h"

#define AFU_WRITE_MEM_ID 1

static const void* handle_opts(int argc, char *argv[], uint64_t *length, bool *valid) {
    *length = 0;
    *valid = true;
    return "";
}

mtl_afu_specification afu_write_mem_specification = {
    AFU_WRITE_MEM_ID,
    "write_mem",

    &handle_opts
};

void afu_write_mem_set_buffer(void *buffer) {

}

uint64_t afu_write_mem_get_written_bytes() {
    // TODO
    return 0;
}
