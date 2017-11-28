#pragma once

typedef enum afu_type {
    AFU_HOST_PASSTHROUGH,

    AFU_BLOWFISH_ENCRYPT,
    AFU_BLOWFISH_DECRYPT,
    AFU_LOWERCASE
} afu_type_t;

typedef struct afu_entry {
    const char *name;
    const afu_type_t key;
} afu_entry_t;

static afu_entry_t afus[] = {
    { .name = "passthrough", .key = AFU_HOST_PASSTHROUGH},
    { .name = "blowfish_encrypt",    .key = AFU_BLOWFISH_ENCRYPT},
    { .name = "blowfish_decrypt",    .key = AFU_BLOWFISH_DECRYPT},
    { .name = "lowercase",   .key = AFU_LOWERCASE},
};
