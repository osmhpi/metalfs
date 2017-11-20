#pragma once

typedef struct afu_entry {
    const char *name;
    const int key;
} afu_entry_t;

static afu_entry_t afus[] = {
    { .name = "blowfish", .key = 0},
    { .name = "sponge"  , .key = 1},
};
