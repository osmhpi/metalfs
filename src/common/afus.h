#pragma once

typedef enum afu_type {
    AFU_HOST_PASSTHROUGH,

    AFU_BLOWFISH,
    AFU_LOWERCASE
} afu_type_t;

typedef struct afu_entry {
    const char *name;
    const afu_type_t key;
} afu_entry_t;

static afu_entry_t afus[] = {
    { .name = "passthrough", .key = AFU_HOST_PASSTHROUGH},
    { .name = "blowfish",    .key = AFU_BLOWFISH},
    { .name = "lowercase",   .key = AFU_LOWERCASE},
};
