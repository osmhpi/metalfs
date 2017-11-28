#pragma once

#include <libsnap.h>

#include <stdint.h>

int blowfish_cipher(struct snap_action *action,
               int mode, unsigned long timeout,
               const uint8_t *ibuf,
               unsigned int in_len,
               uint8_t *obuf,
               unsigned int out_len);


int blowfish_set_key(struct snap_action *action, unsigned long timeout,
                const uint8_t *key, unsigned int length);
