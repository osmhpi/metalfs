#include "operator.h"

#include <stddef.h>
#include <getopt.h>

#include "../../metal/metal.h"

#include "../../metal_fpga/hw/hls/include/action_metalfpga.h"

static const char help[] =
    "Usage: passthrough [-h]\n"
    "\n";

static bool _profile = false;

extern int optind;
static const void* handle_opts(mtl_operator_invocation_args *args, uint64_t *length, bool *valid) {
    optind = 1; // Reset getopt
    _profile = false;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help", no_argument, NULL, 'h' },
            { "profile", no_argument, NULL, 'p' }
        };

        int ch = getopt_long(args->argc, args->argv, "hp", long_options, &option_index);
        if (ch == -1)
            break;

        switch (ch) {
        case 'p':
           _profile = true;
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
    // Nothing to do
    return MTL_SUCCESS;
}

static bool get_profile_enabled() {
    return _profile;
}

mtl_operator_specification op_passthrough_specification = {
    { OP_PASSTHROUGH_ENABLE_ID, OP_PASSTHROUGH_STREAM_ID },
    "passthrough",
    false,

    &handle_opts,
    &apply_config,
    NULL,
    NULL,
    &get_profile_enabled
};
