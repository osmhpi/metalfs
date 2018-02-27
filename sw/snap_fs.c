#define _POSIX_C_SOURCE 200809L //because we want to use posix_memalign, see here: https://stackoverflow.com/questions/32438554/warning-implicit-declaration-of-posix-memalign
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
#include <assert.h>
#include <string.h>
#include <endian.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <action_metalfpga.h>
#include <snap_hls_if.h>

#include "snap_fs.h"

typedef struct {
    uint64_t map;
    mf_extent_t * extents;
    uint16_t extent_count;
} map_options_t;

typedef struct {
    bool query_mapping;
    bool query_state;
    uint64_t lblock;
    uint64_t result_addr;
} query_options_t;

typedef struct {
    uint8_t func_type;
    uint8_t card_no;
    uint8_t slot_no;
    union {
        map_options_t map_opts;
        query_options_t query_opts;
    } func_options;
    uint64_t logical_block;
    uint8_t verbose_flag;
} options_t;

options_t opts;

static void print_memory_64(uint8_t * mem)
{
    for (int i = 0; i < 64; ++i) {
        printf("%02x ", ((uint8_t*)mem)[i]);
        if (i%8 == 7) printf("\n");
    }
}
static void usage(const char *prog)
{
    printf("Usage: %s [-h] [-v, --verbose] [-V, --version]\n"
        "  -c, --card <cardno> (0...3)\n"
        "  -s, --slot <slotno> (0...15)\n"
        "  -u, --unmap\n"
        "  -m, --map <pb_begin>:<pb_count>[,<pb_begin>:<pb_count>]*\n"
        "  -S, --query-state\n"
        "  -M, --query-mapping <lb_number>\n"
        "  -v, --verbose\n"
        "  -h, --help\n"
        "\n",
        prog);
}

static void read_options(int argc, char ** argv)
{
    while (1) {
    int option_index = 0;
    static struct option long_options[] = {
        { "card",          required_argument, NULL, 'c' }, // specify card
        { "slot",          required_argument, NULL, 's' }, // specify file slot to map/unmap
        { "map",           required_argument, NULL, 'm' }, // map extent list onto slot
        { "query-mapping", required_argument, NULL, 'M' }, // query extent and block data from slot
        { "query-state",   no_argument,       NULL, 'S' }, // query state of a slot
        { "unmap",         no_argument,       NULL, 'u' }, // unmap extent list from slot
        { "verbose",       no_argument,       NULL, 'v' }, // do everything loudly
        { "help",          no_argument,       NULL, 'h' }, // print usage
        { 0,               no_argument,       NULL,  0  }, // print usage and exit
    };

    int ch = getopt_long(argc, argv, "c:s:m:M:Suvh", long_options, &option_index);
    if (ch == -1)
        break;

    switch (ch) {
        case 'c':
            opts.card_no = strtol(optarg, (char **)NULL, 0);
            break;
        case 's':
            opts.slot_no = strtol(optarg, (char **)NULL, 0);
            break;
        case 'm':
            opts.func_type = MF_JOB_MAP;
            opts.func_options.map_opts.map = 0xFFFFFFFFFFFFFFFF;
            read_extent_list(&opts.func_options.map_opts.extents, &opts.func_options.map_opts.extent_count, optarg);
            break;
        case 'S':
            opts.func_type = MF_JOB_QUERY;
            opts.func_options.query_opts.query_state = 0xFF;
            // replace with query
            //opts.operation = MF_FILEMAP_MODE_TEST;
            //opts.logical_block = strtol(optarg, (char **)NULL, 0);
            break;
        case 'M':
            opts.func_type = MF_JOB_QUERY;
            opts.func_options.query_opts.query_mapping = 0xFF;
            opts.func_options.query_opts.lblock = strtol(optarg, (char **)NULL, 0);
            // replace with query
            //opts.operation = MF_FILEMAP_MODE_TEST;
            //opts.logical_block = strtol(optarg, (char **)NULL, 0);
            break;
        case 'u':
            opts.func_type = MF_JOB_MAP;
            opts.func_options.map_opts.map = 0;
            break;
        case 'v':
            opts.verbose_flag = 1;
            break;
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
    }
    }

    if (optind != argc) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
}

// tokenizes input, changes input string, only works on NULL-terminated strings. returns first token when first token was found.
static char * tokenize(char *input, char delimeter, char **next_token, int *chars_left) {
    char *returnString = input;
    while (*input != delimeter && *input != '\0' && *chars_left > 0) {
        input++;
        (*chars_left)--;
    }
    if (*input != '\0') {
        *input = '\0';
    }
    *next_token = input + 1;
    (*chars_left)--;
    
    return returnString;
}

// parses extents from input string, extents need to be formatted as <start>:<length>,<start>:length>...
static void read_extent_list(mf_extent_t ** extents, uint16_t * extent_count, char * input)
{
    if (*extents)
    {
        free(*extents);
    }
    *extents = malloc(sizeof(mf_extent_t));
    uint16_t extent_vector_size = 1;
    *extent_count = 0;

    // prepare tokenizer
    const char delim = ',';
    char *next_token;

    // get initial token from input string (format: <extentStart>:<extentLength>)
    int input_left = strlen(input);
    char *token = tokenize(input, delim, &next_token, &input_left);

    while(*token != '\0' && *extent_count < 0xff) //uint16_t limited to 0xff
    {
        (*extent_count)++;

        // if extents gets too small, we double it in size, similar to a C++ style vector
        if (*extent_count > extent_vector_size)
        {
            extent_vector_size *= 2;
            *extents = realloc(*extents, extent_vector_size * sizeof(mf_extent_t));
        }

        // parse extent_start and extent_length from token and store it in extents
        mf_extent_t *new_extent = *extents + (unsigned int)*extent_count - 1;
        char *extent_length;
        int token_length = strlen(token);
        char *extent_start = tokenize(token, ':', &extent_length, &token_length); // rest of token get's put into extent_length, conveniently the rest of the token IS the current extent's length.
        new_extent->block_begin = (uint64_t)strtoull(extent_start, NULL, 10);
        new_extent->block_count = (uint64_t)strtoull(extent_length, NULL, 10);
        
        // get next extent token
        token = tokenize(next_token, delim, &next_token, &input_left);
    }
}

static void snap_prepare_query_job(struct snap_job *cjob, metalfpga_job_t *mjob) {

    assert(sizeof(*mjob) <= 108);
    memset(mjob, 0, sizeof(*mjob));

    mjob->job_address = (uint64_t)snap_malloc(64 * 8); // 1 line needed
    /*if (posix_memalign((void**)&mjob->job_address, 64, sizeof(uint64_t) * 8) != 0) {
        perror("FAILED: posix_memalign");
        return;
    }*/
    uint64_t *job_struct = (uint64_t*)mjob->job_address;
    mjob->job_type = MF_JOB_QUERY;

    // build query job struct
    uint8_t *job_flags = (uint8_t*) job_struct;
    query_options_t *query_opts = &opts.func_options.query_opts;
    job_flags[0] = opts.slot_no;
    job_flags[2] = query_opts->query_state;
    if (query_opts->query_mapping) {
        job_flags[1] = query_opts->query_mapping;
        job_struct[1] = htobe64(query_opts->lblock);
    }

    printf("Preparing QueryJobStruct:\n");
    print_memory_64((uint8_t*)(mjob->job_address));
     
    //move job struct into cjob
    snap_job_set(cjob, mjob, sizeof(*mjob), NULL, 0);
};

static void snap_prepare_map_job(struct snap_job *cjob, metalfpga_job_t *mjob)
{
    assert(sizeof(*mjob) <= 108);
    memset(mjob, 0, sizeof(*mjob));

    map_options_t *map_opts = &opts.func_options.map_opts;
    int lineSize = 64; // 1 line = 64 byte
    int numberOfExtentLines = map_opts->extent_count % 4 ? map_opts->extent_count / 4 + 1 : map_opts->extent_count / 4; // 4 extents per line, can't allocate half lines
    mjob->job_address = (uint64_t)snap_malloc(lineSize * (numberOfExtentLines +1));
    /*if (posix_memalign((void**)&mjob->job_address, 64, lineSize * (numberOfExtentLines + 1) * 4000) != 0) { 
        perror("FAILED: posix_memalign");
        return;
    }*/
    uint64_t *job_struct = (uint64_t*)mjob->job_address;
    mjob->job_type = MF_JOB_MAP;

    // build map job struct
    job_struct[0] = htobe64(opts.slot_no);
    job_struct[1] = htobe64(map_opts->map);
    if (map_opts->map) {
        job_struct[2] = htobe64(map_opts->extent_count);

        for (int i = 0; i < map_opts->extent_count; ++i) {
            job_struct[2 * i + 8] = htobe64(map_opts->extents[i].block_begin);
            job_struct[2 * i + 9] = htobe64(map_opts->extents[i].block_count);
        }
    }

    printf("Preparing MapJobStruct:\n");
    print_memory_64((uint8_t*)(mjob->job_address));
    printf("... extents:\n");
    print_memory_64((uint8_t*)(mjob->job_address)+64);
    // move job struct into cjob
    snap_job_set(cjob, mjob, sizeof(*mjob), NULL, 0);
}

int main(int argc, char *argv[])
{
    int rc = 0;
    struct snap_card *card = NULL;
    struct snap_action *action = NULL;
    char device[128];
    struct snap_job cjob;
    metalfpga_job_t mjob;
    unsigned long timeout = 600;
    int exit_code = EXIT_SUCCESS;
    snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);

    read_options(argc, argv);

    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", opts.card_no);
    card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM,
                               SNAP_DEVICE_ID_SNAP);
    if (card == NULL) {
        fprintf(stderr, "err: failed to open card %u: %s\n",
                opts.card_no, strerror(errno));
        goto out_error;
    }

    action = snap_attach_action(card, METALFPGA_ACTION_TYPE, action_irq, 60);
    if (action == NULL) {
        fprintf(stderr, "err: failed to attach action %u: %s\n",
                opts.card_no, strerror(errno));
        goto out_error1;
    }

    switch(opts.func_type) {
        case MF_JOB_MAP:
            snap_prepare_map_job(&cjob, &mjob);
            break;
        case MF_JOB_QUERY:
            snap_prepare_query_job(&cjob, &mjob);
            break;
    }

    rc = snap_action_sync_execute_job(action, &cjob, timeout);
    if (rc != 0) {
        fprintf(stderr, "err: job execution %d: %s!\n", rc,
                strerror(errno));
        goto out_error2;
    }

    fprintf(stdout, "RETC=%x\n", cjob.retc);

    // output query results
    if (opts.func_type == MF_JOB_QUERY) {
        query_options_t query_opts = opts.func_options.query_opts;
        uint64_t *result = (uint64_t *)mjob.job_address;
        print_memory_64((uint8_t*)(result));
        if (query_opts.query_mapping) {
            printf("Logical block %lu mapped to physical block %lu.\n",
                    query_opts.lblock,
                    be64toh(result[1]));
        }
        if (query_opts.query_state) {
            ((bool)((uint8_t*)result)[3]) ? printf("Slot %d is open ", opts.slot_no) : printf("Slot %d is closed ", opts.slot_no);
            ((bool)((uint8_t*)result)[4]) ? printf("and active.\n") : printf("and not active.\n");
            printf("===== MAPPING =====\n");
            printf("%d extents, ", be16toh((uint16_t)result[2]));
            printf("%lu blocks mapped.\n", be64toh(result[3]));
            printf("===== CURRENT OPERATION =====\n");
            printf("Logical block %lu\n", be64toh(result[4]));
            printf("Physical block %lu\n", be64toh(result[5]));
        }
        //free((uint64_t *)query_opts.result_addr);
    }

    snap_detach_action(action);
    snap_card_free(card);

    exit(exit_code);

out_error2:
    snap_detach_action(action);
    snap_card_free(card);
    exit(EXIT_FAILURE);
out_error1:
    snap_card_free(card);
    exit(EXIT_FAILURE);
out_error:
    exit(EXIT_FAILURE);
}
