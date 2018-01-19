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
    uint8_t card_no;
    uint8_t slot_no;
    uint8_t operation; //MF_FILEMAP_MODEs
    mf_extent_t * extents;
    uint16_t extent_count;
    uint64_t logical_block;
    uint8_t verbose_flag;
} options_t;

options_t opts;

static void usage(const char *prog)
{
	printf("Usage: %s [-h] [-v, --verbose] [-V, --version]\n"
	       "  -C, --card <cardno> (0...3)\n"
	       "  -S, --slot <slotno> (0...15)\n"
	       "  -u, --unmap\n"
	       "  -m, --map <pb_begin>:<pb_count>[,<pb_begin>:<pb_count>]*\n"
	       "  -t, --test <lb_number>\n"
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
            { "card",    required_argument, NULL, 'C' }, // specify card
            { "slot",    required_argument, NULL, 'S' }, // specify file slot to map/unmap
            { "map",     required_argument, NULL, 'm' }, // map extent list onto slot
            { "test",    required_argument, NULL, 't' }, // read data from slot
            { "unmap",   no_argument,       NULL, 'u' }, // unmap extent list from slot
            { "verbose", no_argument,       NULL, 'v' }, // do everything loudly
            { "help",    no_argument,       NULL, 'h' }, // print usage
            { 0,         no_argument,       NULL,  0  }, // print usage and exit
        };

		int ch = getopt_long(argc, argv, "C:S:m:t:uvh", long_options, &option_index);
		if (ch == -1)
			break;

		switch (ch) {
		case 'C':
			opts.card_no = strtol(optarg, (char **)NULL, 0);
			break;
		case 'S':
			opts.slot_no = strtol(optarg, (char **)NULL, 0);
			break;
		case 'm':
			opts.operation = MF_FILEMAP_MODE_MAP;
			read_extent_list(&opts.extents, &opts.extent_count, optarg);
			break;
		case 't':
			opts.operation = MF_FILEMAP_MODE_TEST;
			opts.logical_block = strtol(optarg, (char **)NULL, 0);
			break;
		case 'u':
			opts.operation = MF_FILEMAP_MODE_UNMAP;
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

static void snap_prepare_filemap_job(struct snap_job *cjob, metalfpga_job_t *mjob)
{
	assert(sizeof(*mjob) <= SNAP_JOBSIZE);
	memset(mjob, 0, sizeof(*mjob));

	// build filemap_job struct
	mf_func_filemap_job_t filemap_job;
       	filemap_job.slot = opts.slot_no;
	filemap_job.flags = opts.operation;
	switch (opts.operation) {
	    case MF_FILEMAP_MODE_UNMAP:
		filemap_job.extent_count = 0xff;
		break;
	    case MF_FILEMAP_MODE_MAP:
		filemap_job.extent_count = opts.extent_count;
		if (opts.extent_count <= 5) { 					// direct access. Placing extent list in job struct.
		    memcpy(&filemap_job.extents, opts.extents, sizeof(mf_extent_t) * opts.extent_count);
		} else { 							// indirect access. Placing host memory pointer in job struct.
		    filemap_job.flags |= 0x4;
		    filemap_job.extents.indirect_address = (uint64_t)opts.extents;
		}
		break;
	    case MF_FILEMAP_MODE_TEST:
		filemap_job.extents.indirect_address = opts.logical_block;
		break;
	}

	// set job struct parameters
	mjob->function = MF_FUNC_FILEMAP;
	memcpy(&mjob->jspec.filemap, (const void *)&filemap_job, sizeof(mf_func_filemap_job_t));

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

	snap_prepare_filemap_job(&cjob, &mjob);

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
