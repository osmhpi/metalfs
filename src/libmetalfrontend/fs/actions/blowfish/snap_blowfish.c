/*
 * Example Blowfish Implementation for HLS tryout
 *
 * Experimental, use for real data not reccomended
 *
 * Wikipedia's pages are based on "CC BY-SA 3.0"
 * Creative Commons Attribution-ShareAlike License 3.0
 * https://creativecommons.org/licenses/by-sa/3.0/
 */

/*
 * Copyright 2017 International Business Machines
 * Copyright 2017 Lukas Wenzel, HPI
 * Copyright 2017 Balthasar Martin, HPI
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <snap_tools.h>
#include <libsnap.h>
#include "action_blowfish.h"
#include <snap_hls_if.h>

static const char *version = "GIT_VERSION";
int verbose_flag = 0;

/*
 * Blowfish: a symmetric-key block cipher (https://en.wikipedia.org/wiki/Blowfish_(cipher))
 *    Simple demo to encrypt and decrypt with blowfish in ECB-mode.
 *
 *
 * Implementation:
 *    We ask FPGA to visit the host memory to traverse this data structure.
 *    1. We need to set a BFS_ACTION_TYPE, this is the ACTION ID.
 *    2. We need to fill in 108 bytes configuration space.
 *    Host will send this field to FPGA via MMIO-32.
 *          This field is completely user defined. see 'bfs_job_t'
 *    3. Call snap APIs
 *
 * Notes:
 *    When 'timeout' is reached, PSLSE will send ha_jcom=LLCMD (0x45) and uncompleted transactions will be killed.
 *
 */

/*---------------------------------------------------
 *       Sample Data
 *---------------------------------------------------*/

/*
 * FIXME If you like to use those pointers directly, use an gcc's alignment
 *       attributes and ensure that it is 128 byte aligned.
 * FIXME Use a constant instead of the 128 ...
 *
 * E.g. like this:
 *   __attribute__((align(128)));
 */
static const uint8_t example_plaintext[] __attribute__((aligned(128))) = {
    0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, /*  8 bytes */
    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, /* 16 bytes */
    0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, /* 24 bytes */
    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, /* 32 bytes */
    0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, /* 40 bytes */
    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, /* 48 bytes */
    0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, /* 56 bytes */
    0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, /* 64 bytes */
};

static uint8_t example_encrypted[64] __attribute__((aligned(128))) = {
    0x00,
};

static uint8_t example_decrypted[64] __attribute__((aligned(128))) = {
    0x00,
};

static uint8_t example_key[] __attribute__((aligned(128))) = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};

/*---------------------------------------------------
 *       Hook 108B Configuration
 *---------------------------------------------------*/

static void snap_prepare_blowfish(struct snap_job *job,
        uint32_t mode_in,
        uint32_t data_length_in,
        blowfish_job_t *bjob_in,
        blowfish_job_t *bjob_out,
        const void *addr_in,
        uint16_t type_in,
        void *addr_out,
        uint16_t type_out)
{
    static const char *mode_str[] =
	    { "MODE_SET_KEY", "MODE_ENCRYPT", "MODE_DECRYPT" };

    fprintf(stderr, "----------------  Config Space ----------- \n");
    fprintf(stderr, "mode = %d %s\n", mode_in, mode_str[mode_in % 3]);
    fprintf(stderr, "input_address = %p -> ", addr_in);
    fprintf(stderr, "output_address = %p -> ", addr_out);
    fprintf(stderr, "data_length = %d\n", data_length_in);
    fprintf(stderr, "------------------------------------------ \n");

    snap_addr_set(&bjob_in->input_data, addr_in, data_length_in,
		  type_in, SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_SRC);

    snap_addr_set(&bjob_in->output_data, addr_out, data_length_in,
		  type_out, SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_DST |
		  SNAP_ADDRFLAG_END );

    bjob_in->mode = mode_in;
    bjob_in->data_length = data_length_in;

    // Here sets the 108byte MMIO settings input.
    // We have input parameters.
    snap_job_set(job, bjob_in, sizeof(*bjob_in),
		 bjob_out, sizeof(*bjob_out));
}

/*---------------------------------------------------
 *       MAIN
 *---------------------------------------------------*/

int blowfish_set_key(struct snap_action *action, unsigned long timeout,
                const uint8_t *key, unsigned int length)
{
    int rc = 0;
    struct timeval etime, stime;
    struct snap_job job;
    blowfish_job_t bjob_in;

    // input validation
    if (length < 4 || length > 56 || length % 4 != 0) {
        printf("err: key has to be multiple of 4 bytes with a length "
               "between 4 and 56 bytes!\n");
        return -EINVAL;
        }

    snap_prepare_blowfish(&job, MODE_SET_KEY, length, &bjob_in, NULL,
                  key, SNAP_ADDRTYPE_HOST_DRAM,
                  NULL, SNAP_ADDRTYPE_UNUSED);

    gettimeofday(&stime, NULL);
    rc = snap_action_sync_execute_job(action, &job, timeout);
    gettimeofday(&etime, NULL);
    if (rc != 0) {
        fprintf(stderr, "err: job execution %d: %s!\n", rc,
            strerror(errno));
        goto out_error;
    }

    fprintf(stderr, "RETC=%x\n", job.retc);
    fprintf(stderr, "INFO: Blowfish took %lld usec\n\n",
        (long long)timediff_usec(&etime, &stime));

    if (verbose_flag) {
	fprintf(stderr, "------------------------------------------ \n");
	fprintf(stderr, "Key set to:\n");
	__hexdump(stderr, key, length);
    }

    return 0;

 out_error:
    return -EIO;
}

int blowfish_cipher(struct snap_action *action,
               int mode, unsigned long timeout,
               const uint8_t *ibuf,
               unsigned int in_len,
               uint8_t *obuf,
               unsigned int out_len)
{
    int rc = 0;
    struct timeval etime, stime;
    struct snap_job job;
    blowfish_job_t bjob_in;

    if (in_len % 8 != 0 || in_len <= 0) {
        printf("err: data to en- or decrypt has to be multiple of "
               "8 bytes and at least 8 bytes long!\n");
        return -EINVAL;
        }

    snap_prepare_blowfish(&job, mode, in_len, &bjob_in, NULL,
                  (void *)ibuf, SNAP_ADDRTYPE_HOST_DRAM,
                  (void *)obuf, SNAP_ADDRTYPE_HOST_DRAM);

    gettimeofday(&stime, NULL);
    rc = snap_action_sync_execute_job(action, &job, timeout);
    gettimeofday(&etime, NULL);
    if (rc != 0) {
        fprintf(stderr, "err: job execution %d: %s!\n", rc,
            strerror(errno));
        goto out_error;
    }

    fprintf(stderr, "RETC=%x\n", job.retc);
    fprintf(stderr, "INFO: Blowfish took %lld usec\n",
        (long long)timediff_usec(&etime, &stime));

    if (verbose_flag) {
	fprintf(stderr, "------------------------------------------ \n");
	fprintf(stderr, "Input Buffer:\n");
	__hexdump(stderr, ibuf, in_len);
	fprintf(stderr, "Output Buffer:\n");
	__hexdump(stderr, obuf, out_len);
    }

    return 0;

 out_error:
    return -EIO;
}

static inline
ssize_t file_size(const char *fname)
{
        int rc;
        struct stat s;

        rc = lstat(fname, &s);
        if (rc != 0) {
                fprintf(stderr, "err: Cannot find %s!\n", fname);
                return rc;
        }
        return s.st_size;
}

static inline ssize_t
file_read(const char *fname, uint8_t *buff, size_t len)
{
        int rc;
        FILE *fp;

        if ((fname == NULL) || (buff == NULL) || (len == 0))
                return -EINVAL;

        fp = fopen(fname, "r");
        if (!fp) {
                fprintf(stderr, "err: Cannot open file %s: %s\n",
                        fname, strerror(errno));
                return -ENODEV;
        }
        rc = fread(buff, len, 1, fp);
        if (rc == -1) {
                fprintf(stderr, "err: Cannot read from %s: %s\n",
                        fname, strerror(errno));
                fclose(fp);
                return -EIO;
        }

        fclose(fp);
        return rc;
}

static inline ssize_t
file_write(const char *fname, const uint8_t *buff, size_t len)
{
        int rc;
        FILE *fp;

        if ((fname == NULL) || (buff == NULL) || (len == 0))
                return -EINVAL;

        fp = fopen(fname, "w+");
        if (!fp) {
                fprintf(stderr, "err: Cannot open file %s: %s\n",
                        fname, strerror(errno));
                return -ENODEV;
        }
        rc = fwrite(buff, len, 1, fp);
        if (rc == -1) {
                fprintf(stderr, "err: Cannot write to %s: %s\n",
                        fname, strerror(errno));
                fclose(fp);
                return -EIO;
        }

        fclose(fp);
        return rc;
}

static int blowfish_test(struct snap_action *action, unsigned long timeout)
{
    int rc;

    /* Set a key */
    rc = blowfish_set_key(action, timeout, example_key, sizeof(example_key));
    if (rc != 0)
        return -1;

    /* Encrypt data */
    rc = blowfish_cipher(action, MODE_ENCRYPT, timeout,
                 example_plaintext, sizeof(example_plaintext),
                 example_encrypted, sizeof(example_encrypted));
    if (rc != 0)
        return -2;

    /* Decrypt data */
    rc = blowfish_cipher(action, MODE_DECRYPT, timeout,
                 example_encrypted, sizeof(example_encrypted),
                 example_decrypted, sizeof(example_decrypted));
    if (rc != 0)
        return -3;

    /* Verification */
    if (memcmp(example_plaintext, example_decrypted,
           sizeof(example_plaintext)) != 0) {
        fprintf(stderr, "ERROR: Data does not match!!\n");
        return -4;
    }
    return 0;
}

/**
 * @brief       Print valid command line options
 * @param prog  Current program name
 */
static void usage(const char *prog)
{
        printf("Usage: %s [-h] [-v, --verbose] [-V, --version]\n"
               "  -C, --card <cardno> can be (0...3)\n"
               "  -i, --input <file.bin>    input\n"
               "  -o, --output <file.bin>   output\n"
               "  -k, --key <file.bin>      key\n"
	       "  -d, --decrypt\n"
	       "  -d, --decrypt\n"
               "  -t, --timeout             timeout (sec)"
               "\n"
               "Example:\n"
               "  snap_blowfish ...\n"
               "\n",
               prog);
}

// int main(int argc, char *argv[])
// {
//     int rc;
//     int card_no = 0;
//     const char *input = NULL;
//     const char *output = NULL;
//     const char *key = NULL;
//     uint8_t *ibuf = NULL, *obuf = NULL, *kbuf = NULL;
//     size_t ilen, klen;
//     int decrypt = 0;
//     int test = 0;
//     unsigned long timeout = 10000;
//     struct snap_card *card = NULL;
//     struct snap_action *action = NULL;
//     snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
//     char device[128];
//     int ch;

//     while (1) {
// 	int option_index = 0;
// 	static struct option long_options[] = {
// 		{ "card",        required_argument, NULL, 'C' },
// 		{ "input",       required_argument, NULL, 'i' },
// 		{ "output",      required_argument, NULL, 'o' },
// 		{ "key",         required_argument, NULL, 'k' },
// 		{ "test",        no_argument,       NULL, 'T' },
// 		{ "decrypt",     no_argument,       NULL, 'd' },
// 		{ "timeout",     required_argument, NULL, 't' },
// 		{ "version",     no_argument,       NULL, 'V' },
// 		{ "verbose",     no_argument,       NULL, 'v' },
// 		{ "help",        no_argument,       NULL, 'h' },
// 		{ 0,             no_argument,       NULL, 0   },
// 	};

// 	ch = getopt_long(argc, argv, "C:i:o:k:Tdt:Vvh",
// 			 long_options, &option_index);
// 	if (ch == -1)
// 		break;

// 	switch (ch) {
// 	case 'C':
// 		card_no = strtol(optarg, (char **)NULL, 0);
// 		break;
// 	case 'i':
// 		input = optarg;
// 		break;
// 	case 'o':
// 		output = optarg;
// 		break;
// 	case 'k':
// 		key = optarg;
// 		break;
// 	case 'T':
// 		test = 1;
// 		break;
// 	case 'd':
// 		decrypt = 1;
// 		break;
// 	case 't':
// 		timeout = strtol(optarg, (char **)NULL, 0);
// 		break;
// 	case 'V':
// 		printf("%s\n", version);
// 		exit(EXIT_SUCCESS);
// 	case 'v':
// 		verbose_flag++;
// 		break;
// 	case 'h':
// 		usage(argv[0]);
// 		exit(EXIT_SUCCESS);
// 		break;
// 	default:
// 		usage(argv[0]);
// 		exit(EXIT_FAILURE);
// 	}
//     }

//     fprintf(stderr, "Blowfish cipher\n"
// 	    "  operation: %s input: %s output: %s key: %s\n",
// 	    decrypt ? "decrypt" : "encrypt", input, output, key);

//     //////////////////////////////////////////////////////////////////////

//     fprintf(stderr, "snap_kernel_attach start...\n");
//     snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", card_no);
//     card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM,
//                    SNAP_DEVICE_ID_SNAP);
//     if (card == NULL) {
//         fprintf(stderr, "err: failed to open card %u: %s\n",
//             card_no, strerror(errno));
//         goto out_error;
//     }

//     action = snap_attach_action(card, BLOWFISH_ACTION_TYPE, action_irq, 60);
//     if (action == NULL) {
//         fprintf(stderr, "err: failed to attach action %u: %s\n",
//             card_no, strerror(errno));
//         goto out_error1;
//     }

//     if (test) {
//         rc = blowfish_test(action, timeout);
//         if (rc == 0)
//             goto go_home;
//         else
//             goto out_error3;
//     }

//     ilen = file_size(input);
//     if (ilen <= 0)
//         goto out_error2;

//     klen = file_size(key);
//     if (klen <= 0)
//         goto out_error2;

//     ibuf = snap_malloc(ilen);
//     if (ibuf == NULL)
//         goto out_error3;

//     obuf = snap_malloc(ilen);
//     if (obuf == NULL)
//         goto out_error3;

//     kbuf = snap_malloc(klen);
//     if (kbuf == NULL)
//         goto out_error3;

//     rc = file_read(input, ibuf, ilen);
//     if (rc <= 0)
//         goto out_error3;

//     rc = file_read(key, kbuf, klen);
//     if (rc <= 0)
//         goto out_error3;

//     /* Set a key */
//     rc = blowfish_set_key(action, timeout, kbuf, klen);
//     if (rc != 0)
//         goto out_error3;

//     /* Encrypt/decrypt data */
//     rc = blowfish_cipher(action, decrypt ? MODE_DECRYPT : MODE_ENCRYPT,
//                  timeout, ibuf, ilen, obuf, ilen);
//     if (rc != 0)
//         goto out_error3;

//     rc = file_write(output, obuf, ilen);
//     if (rc <= 0)
//         goto out_error3;

//  go_home:
//     __free(ibuf);
//     __free(obuf);
//     __free(kbuf);

//     snap_detach_action(action);
//     snap_card_free(card);
//     exit(EXIT_SUCCESS);

//  out_error3:
//     __free(ibuf);
//     __free(obuf);
//     __free(kbuf);
//  out_error2:
//     snap_detach_action(action);
//  out_error1:
//     snap_card_free(card);
//  out_error:
//     exit(EXIT_FAILURE);
// }
