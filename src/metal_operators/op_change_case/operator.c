#include "operator.h"

#include <stddef.h>
#include <getopt.h>
#include <endian.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

#include "../../metal_fpga/include/action_metalfpga.h"

#include "../../metal/metal.h"

static const char help[] =
    "Usage: change_case [-h]\n"
    "  -l, --lowercase              transform to lowercase\n"
    "\n"
    "Example:\n"
    "----------\n"
    "rm /tmp/t2\n"
    "echo \"Hello world. This is my first CAPI SNAP experience. It's real fun!\n\" > /tmp/t1\n"
    "\n"
    "cat /tmp/t1 | ./change_case > /tmp/t2\n"
    "\n"
    "echo \"Display input file\"; cat /tmp/t1\n"
    "Hello world. This is my first CAPI SNAP experience. It's real fun!\n"
    "echo \"Display output file from FPGA EXECUTED ACTION\"; cat /tmp/t2\n"
    "HELLO WORLD. THIS IS MY FIRST CAPI SNAP EXPERIENCE. IT'S REAL FUN!\n"
    "\n";

static uint64_t _mode = 0;
static bool _profile = 0;

extern int optind;
static const void* handle_opts(mtl_operator_invocation_args *args, uint64_t *length, bool *valid) {
    optind = 1; // Reset getopt
    _mode = 0;
    _profile = 0;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help", no_argument, NULL, 'h' },
            { "lowercase", no_argument, NULL, 'l' },
            { "profile", no_argument, NULL, 'p' },
        };

        int ch = getopt_long(args->argc, args->argv, "lhp", long_options, &option_index);
        if (ch == -1)
            break;

        switch (ch) {
        case 'l':
            _mode = 1;
            break;
        case 'p':
            _profile = 1;
            break;
        case 'h':
        default:
            *length = sizeof(help);
            *valid = false;
            return help;
        }
    }

    *valid = true;
    *length = 0;
    return "";
}

static int apply_config(struct snap_action *action) {
    uint64_t *job_struct = (uint64_t*)snap_malloc(2 * sizeof(uint64_t));
    job_struct[0] = htobe64(_mode);

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_OP_CHANGE_CASE_SET_MODE;
    mjob.job_address = (uint64_t)job_struct;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(action, &cjob, timeout);

    free(job_struct);

    if (rc != 0)
        // Some error occurred
        return MTL_ERROR_INVALID_ARGUMENT;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return MTL_ERROR_INVALID_ARGUMENT;

    return MTL_SUCCESS;
}

static bool get_profile_enabled() {
    return _profile;
}

mtl_operator_specification op_change_case_specification = {
    { OP_CHANGE_CASE_ENABLE_ID, OP_CHANGE_CASE_STREAM_ID },
    "change_case",
    false,

    &handle_opts,
    &apply_config,
    NULL,
    NULL,
    &get_profile_enabled
};
