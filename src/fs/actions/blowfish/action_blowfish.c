/*
 * Copyright 2017 International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Example to use the FPGA to find patterns in a byte-stream.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libsnap.h>
#include <linux/types.h>	/* __be64 */
#include <asm/byteorder.h>
#include <arpa/inet.h>

#include <snap_internal.h>
#include <snap_tools.h>
#include "action_blowfish.h"
#include "action_blowfish_data.h"

static uint32_t g_P[18];
static uint32_t g_S[4][256];

static uint32_t bf_f (uint32_t *x) {
   uint32_t h = g_S[0][*x >> 24] + g_S[1][*x >> 16 & 0xff];
   return ( h ^ g_S[2][*x >> 8 & 0xff] ) + g_S[3][*x & 0xff];
}

static void bf_encrypt (uint32_t *L, uint32_t *R) {
   for (int i=0 ; i<16 ; i += 2) {
      *L ^= g_P[i];
      *R ^= bf_f(L);
      *R ^= g_P[i+1];
      *L ^= bf_f(R);
   }
   *L ^= g_P[16];
   *R ^= g_P[17];

   uint32_t tmp = *L;
   *L = *R;
   *R = tmp;
}

static void bf_decrypt (uint32_t *L, uint32_t *R) {
   for (int i=16 ; i > 0 ; i -= 2) {
      *L ^= g_P[i+1];
      *R ^= bf_f(L);
      *R ^= g_P[i];
      *L ^= bf_f(R);
   }
   *L ^= g_P[1];
   *R ^= g_P[0];

   uint32_t tmp = *L;
   *L = *R;
   *R = tmp;
}

static void bf_endecrypt(int decrypt, uint64_t *input, uint64_t *output, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        // Calculate the addresses for left and right inputs
        uint64_t *currentInputBlock = input + i;
        uint32_t *leftInput = (uint32_t*)currentInputBlock;
        uint32_t *rightInput = leftInput + 1;

        // Adjust byte order to big endian
        uint32_t left = htonl(*leftInput);
        uint32_t right = htonl(*rightInput);

        if (decrypt)
            bf_decrypt(&left, &right);
        else
            bf_encrypt(&left, &right);

        // Determine where to write the results
        uint64_t *currentOutputBlock = output + i;
        uint32_t *leftOutput = (uint32_t*)currentOutputBlock;
        uint32_t *rightOutput = leftOutput + 1;

        // Restore byte order if necessary
        *leftOutput = ntohl(left);
        *rightOutput = ntohl(right);
    }
}

static void bf_keyInit(uint32_t (*key)[18])
{
    for (int i = 0; i < 18; ++i) {
        g_P[i] = c_initP[i] ^ (*key)[i];
    }
    for (int n = 0; n < 4; ++n) {
        for (int i = 0; i < 256; ++i) {
            g_S[n][i] = c_initS[n][i];
        }
    }

    uint32_t left = 0, right = 0;
    for (int i = 0; i < 18; i += 2) {
        bf_encrypt(&left, &right);
        g_P[i] = left;
        g_P[i+1] = right;
    }
    for (int n = 0; n < 4; ++n)
    {
        for (int i = 0; i < 256; i += 2)
        {
            bf_encrypt(&left, &right);
            g_S[n][i] = left;
            g_S[n][i+1] = right;
        }
    }
}

static int action_main(struct snap_sim_action *action,
               void *job, unsigned int job_len)
{
    (void)job_len;

    struct blowfish_job *js = (struct blowfish_job *)job;

    // if (sizeof(*js) != job_len)
    // 	goto err_out;

    switch (js->mode) {
    case MODE_SET_KEY: {
        uint32_t keyLine[512>>2] = {0};
        uint32_t key[18] = { 0 };
        memcpy(keyLine, (void*)js->input_data.addr, js->data_length);
        size_t keyWords = js->data_length >> 2;
        for (size_t i = 0; i < 18; ++i) {
            key[i] = htonl(keyLine[i % keyWords]);
        }
        bf_keyInit(&key);
        break;
    }
    case MODE_ENCRYPT: {
        bf_endecrypt(0, (uint64_t*)js->input_data.addr, (uint64_t*)js->output_data.addr, js->data_length / sizeof(uint64_t));
        break;
    }
    case MODE_DECRYPT: {
        bf_endecrypt(1, (uint64_t*)js->input_data.addr, (uint64_t*)js->output_data.addr, js->data_length / sizeof(uint64_t));
        break;
    }
    }

    action->job.retc = SNAP_RETC_SUCCESS;
    return 0;
//  err_out:
// 	action->job.retc = SNAP_RETC_FAILURE;
// 	return 0;
}

static struct snap_sim_action action = {
    .vendor_id = SNAP_VENDOR_ID_ANY,
    .device_id = SNAP_DEVICE_ID_ANY,
    .action_type = BLOWFISH_ACTION_TYPE,

    .job = { .retc = SNAP_RETC_FAILURE, },
    .state = ACTION_IDLE,
    .main = action_main,
    .priv_data = NULL,
    .mmio_write32 = NULL,
    .mmio_read32 = NULL,

    .next = NULL,
};

static void _init(void) __attribute__((constructor));

static void _init(void)
{
    snap_action_register(&action);
}
