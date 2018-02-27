#include "afu.h"

#define AFU_READ_MEM_ID 0

static const void* handle_opts(int argc, char *argv[], uint64_t *length, bool *valid) {
    *length = 0;
    *valid = true;
    return "";
}

mtl_afu_specification afu_read_mem_specification = {
    AFU_READ_MEM_ID,
    "read_mem",

    &handle_opts
};

void afu_read_mem_set_buffer(void *buffer, uint64_t length) {

}
