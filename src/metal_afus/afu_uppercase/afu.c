#include "afu.h"

#include <stddef.h>
#include <getopt.h>

#define AFU_UPPERCASE_ID 2

const char help[] =
    "Usage: uppercase [-h]\n"
    "\n"
    "Example:\n"
    "----------\n"
    "rm /tmp/t2\n"
    "echo \"Hello world. This is my first CAPI SNAP experience. It's real fun!\n\" > /tmp/t1\n"
    "\n"
    "cat /tmp/t1 | ./uppercase > /tmp/t2\n"
    "\n"
    "echo \"Display input file\"; cat /tmp/t1\n"
    "Hello world. This is my first CAPI SNAP experience. It's real fun!\n"
    "echo \"Display output file from FPGA EXECUTED ACTION\"; cat /tmp/t2\n"
    "HELLO WORLD. THIS IS MY FIRST CAPI SNAP EXPERIENCE. IT'S REAL FUN!\n"
    "\n";

static const void* handle_opts(int argc, char *argv[], uint64_t *length, bool *valid) {
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help", no_argument, NULL, 'h' }
        };

        int ch = getopt_long(argc, argv, "h", long_options, &option_index);
        if (ch == -1)
            break;

        switch (ch) {
        case 'h':
        default:
            *length = sizeof(help);
            *valid = false;
            return help;
        }
    }

    *length = 0;
    return "";
}

static const int apply_config() {

}

mtl_afu_specification afu_uppercase_specification = {
    AFU_UPPERCASE_ID,
    "uppercase",

    &handle_opts,
    &apply_config
};
