
extern "C" {
#include "../../metal_filesystem/metal.h"
};

#include <metal_filesystem_pipeline/metal_pipeline_storage.hpp>
#include <thread>
#include <libgen.h>
#include <dirent.h>
#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include "server.hpp"
#include "../../../third_party/cxxopts/include/cxxopts.hpp"
#include "metal_fuse_operations.hpp"

struct metal_config {
  int card;
  char *operators;
  char *metadata_dir;
  int in_memory;
};
enum {
  KEY_HELP,
  KEY_VERSION,
};

#define METAL_OPT(t, p, v) { t, offsetof(struct metal_config, p), v }

static struct fuse_opt metal_opts[] = {
        METAL_OPT("--card=%i",         card, 0),
        METAL_OPT("-c %i",             card, 0),
        METAL_OPT("--metadata %s",     metadata_dir, 0),
        METAL_OPT("--in_memory",       in_memory, 1),
//        METAL_OPT("nomybool",          in_memory, 0),
        METAL_OPT("--in_memory=true",  in_memory, 1),
        METAL_OPT("--in_memory=false", in_memory, 0),

        FUSE_OPT_KEY("-V",             KEY_VERSION),
        FUSE_OPT_KEY("--version",      KEY_VERSION),
        FUSE_OPT_KEY("-h",             KEY_HELP),
        FUSE_OPT_KEY("--help",         KEY_HELP),
        FUSE_OPT_END
};

static int metal_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
    (void)data;
    (void)arg;

    switch (key) {
        case KEY_HELP:
            fprintf(stderr,
                    "usage: %s mountpoint [options]\n"
                    "\n"
                    "general options:\n"
                    "    -o opt,[opt...]  mount options\n"
                    "    -h   --help      print help\n"
                    "    -V   --version   print version\n"
                    "\n"
                    "Myfs options:\n"
                    "    -o card=CARD (0)\n"
                    "    -o mystring=STRING\n"
                    "    -o mybool\n"
                    "    -o nomybool\n"
                    "    -n NUM           same as '-omynum=NUM'\n"
                    "    --mybool=BOOL    same as 'mybool' or 'nomybool'\n"
                    , outargs->argv[0]);
        fuse_opt_add_arg(outargs, "-h");
        fuse_main(outargs->argc, outargs->argv, &metal::metal_fuse_operations, NULL);
        exit(1);

        case KEY_VERSION:
            fprintf(stderr, "Metal version %s\n", "0.0.1");
        fuse_opt_add_arg(outargs, "--version");
        fuse_main(outargs->argc, outargs->argv, &metal::metal_fuse_operations, NULL);
        exit(0);
    }
    return 1;
}

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct metal_config conf;
    memset(&conf, 0, sizeof(conf));

    if (fuse_opt_parse(&args, &conf, metal_opts, metal_opt_proc)) {
        return 1;
    }

    // spdlog::set_level(spdlog::level::debug);

    auto & c = metal::Context::instance();

    c.initialize(
            static_cast<bool>(conf.in_memory),
            std::string(argv[0]),
            std::string(conf.metadata_dir),
            conf.card
    );

    auto server = std::thread(metal::Server::start, c.socket_filename(), c.registry(), c.card());

    spdlog::info("Starting FUSE driver...");
    auto retc = fuse_main(args.argc, args.argv, &metal::metal_fuse_operations, nullptr);

    // This de-allocates the action/card, so this must be called every time we exit
    mtl_deinitialize(metal::Context::instance().storage());

    return retc;
}
