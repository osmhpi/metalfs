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

#include <snap_tools.h>
#include <libsnap.h>
#include <action_metalfpga.h>
#include <snap_hls_if.h>

#include "snap_fs.h"

typedef struct {
    uint8_t map;
    mf_extent_t * extents;
    uint16_t extent_count;
} map_options_t;

typedef struct {
    bool query_mapping;
    bool query_state;
    uint64_t lblock;
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
            opts.func_type = MF_FUNC_MAP;
            opts.func_options.map_opts.map = true;
            read_extent_list(&opts.func_options.map_opts.extents, &opts.func_options.map_opts.extent_count, optarg);
            break;
        case 'S':
            opts.func_type = MF_FUNC_QUERY;
            opts.func_options.query_opts.query_state = true;
            // replace with query
            //opts.operation = MF_FILEMAP_MODE_TEST;
            //opts.logical_block = strtol(optarg, (char **)NULL, 0);
            break;
        case 'M':
            opts.func_type = MF_FUNC_QUERY;
            opts.func_options.query_opts.query_mapping = true;
            opts.func_options.query_opts.lblock = strtol(optarg, (char **)NULL, 0);
            // replace with query
            //opts.operation = MF_FILEMAP_MODE_TEST;
            //opts.logical_block = strtol(optarg, (char **)NULL, 0);
            break;
        case 'u':
            opts.func_type = MF_FUNC_MAP;
            opts.func_options.map_opts.map = false;
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

    assert(sizeof(*mjob) <= SNAP_JOBSIZE);
    memset(mjob, 0, sizeof(*mjob));

    // build query job struct
    mf_func_query_job_t query_job;
    query_options_t *query_opts = &opts.func_options.query_opts;
    query_job.slot = opts.slot_no;
    if (query_opts->query_mapping) {
        query_job.query_mapping = true;
        query_job.query_mapping = query_opts->lblock;
    }
    query_job.query_state = query_opts->query_state;

    //set job struct parameters
    mjob->function = MF_FUNC_QUERY;
    memcpy(&mjob->jspec.query, (const void *)&query_job, sizeof(mf_func_query_job_t));

    //move job struct into cjob
    snap_job_set(cjob, mjob, sizeof(*mjob), NULL, 0);
};

static void snap_prepare_map_job(struct snap_job *cjob, metalfpga_job_t *mjob)
{
    assert(sizeof(*mjob) <= SNAP_JOBSIZE);
    memset(mjob, 0, sizeof(*mjob));

    // build map job struct
    mf_func_map_job_t map_job;
    map_options_t *map_opts = &opts.func_options.map_opts;
    map_job.slot = opts.slot_no;
    map_job.map = map_opts->map;
    switch (map_opts->map) {
        case false:
            map_job.indirect = false;
            map_job.extent_count = 0xff;
            break;
        case true:
            map_job.extent_count = map_opts->extent_count;
            if (map_opts->extent_count <= 5) { 					// direct access. Placing extent list in job struct.
                map_job.indirect = false;
                memcpy(&map_job.direct_extents, map_opts->extents, sizeof(mf_extent_t) * map_opts->extent_count);
            } else { 							// indirect access. Placing host memory pointer in job struct.
                map_job.indirect = true;
                map_job.indirect_address = (uint64_t)map_opts->extents;
            }
            break;
    }

    // set job struct parameters
    mjob->function = MF_FUNC_MAP;
    memcpy(&mjob->jspec.map, (const void *)&map_job, sizeof(mf_func_map_job_t));

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
        case MF_FUNC_MAP:
            snap_prepare_map_job(&cjob, &mjob);
            break;
        case MF_FUNC_QUERY:
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
    if (cjob.retc != SNAP_RETC_SUCCESS) {
        fprintf(stderr, "err: Unexpected RETC=%x!\n", cjob.retc);
        goto out_error2;
    }

    snap_detach_action(action);
    snap_card_free(card);

    exit(exit_code);

out_error2:
    snap_detach_action(action);
out_error1:
    snap_card_free(card);
out_error:
    exit(EXIT_FAILURE);
}
