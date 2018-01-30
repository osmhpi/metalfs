#include <snap_tools.h>
#include <libsnap.h>
#include <action_metalfpga.h>
#include <snap_hls_if.h>

typedef struct mf_extent {
	uint64_t block_begin;
	uint64_t block_count;
} mf_extent_t;

static void usage(const char *prog);
static void read_extent_list(mf_extent_t ** extents, uint16_t * extent_count, char * input);
static void read_options(int argc, char ** argv);
static void snap_prepare_map_job(struct snap_job *cjob, metalfpga_job_t *mjob);
static void snap_prepare_query_job(struct snap_job *cjob, metalfpga_job_t *mjob);
static char *tokenize(char *input, char delimeter, char **next_token, int *chars_left);

