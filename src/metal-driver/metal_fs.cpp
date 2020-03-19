extern "C" {
#include <metal-filesystem/metal.h>
}

#include <thread>

#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

#include <metal-filesystem-pipeline/filesystem_context.hpp>
#include <metal-pipeline/operator_factory.hpp>
#include <metal-pipeline/snap_action.hpp>

#include "filesystem_fuse_handler.hpp"
#include "metal_fuse_operations.hpp"
#include "operator_fuse_handler.hpp"
#include "pseudo_operators.hpp"
#include "server.hpp"
#include "socket_fuse_handler.hpp"

using namespace metal;

struct metal_config {
  int card;
  int timeout;
  char *operators;
  char *metadata_dir;
  int in_memory;
  int verbosity;
};
enum {
  KEY_HELP,
  KEY_VERSION,
};

#define METAL_OPT(t, p, v) \
  { t, offsetof(struct metal_config, p), v }

static struct fuse_opt metal_opts[] = {
    METAL_OPT("--card=%i", card, 0),
    METAL_OPT("-c %i", card, 0),
    METAL_OPT("--timeout=%i", timeout, 10),
    METAL_OPT("-t %i", timeout, 10),
    METAL_OPT("--metadata %s", metadata_dir, 0),
    METAL_OPT("--in-memory", in_memory, 1),
    METAL_OPT("--in-memory=true", in_memory, 1),
    METAL_OPT("--in-memory=false", in_memory, 0),
    METAL_OPT("-v", verbosity, 1),
    METAL_OPT("-vv", verbosity, 2),
    METAL_OPT("-vvv", verbosity, 3),

    FUSE_OPT_KEY("-V", KEY_VERSION),
    FUSE_OPT_KEY("--version", KEY_VERSION),
    FUSE_OPT_KEY("-h", KEY_HELP),
    FUSE_OPT_KEY("--help", KEY_HELP),
    FUSE_OPT_END};

static int metal_opt_proc(void *data, const char *arg, int key,
                          struct fuse_args *outargs) {
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
              "metal_fs options:\n"
              "    --card=CARD (0)\n"
              "    --timeout=TIMEOUT (10)\n"
              "    --metadata=METADATA_PATH\n"
              "    --in-memory=(true|false)\n",
              outargs->argv[0]);
      fuse_opt_add_arg(outargs, "-h");
      fuse_main(outargs->argc, outargs->argv, &Context::fuseOperations(), NULL);
      exit(1);
      break;

    case KEY_VERSION:
      fprintf(stderr, "metal_fs version %s\n", "0.0.1");
      fuse_opt_add_arg(outargs, "--version");
      fuse_main(outargs->argc, outargs->argv, &Context::fuseOperations(), NULL);
      exit(0);
      break;
  }
  return 1;
}

class InMemoryFilesystem : public FilesystemContext {
 public:
  InMemoryFilesystem(std::string metadataDir,
                     bool deleteMetadataIfExists = false)
      : FilesystemContext(metadataDir, deleteMetadataIfExists) {
    mtl_initialize(&_context, metadataDir.c_str(), &in_memory_storage);
  }
};

int main(int argc, char *argv[]) {
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  struct metal_config conf;
  memset(&conf, 0, sizeof(conf));

  if (fuse_opt_parse(&args, &conf, metal_opts, metal_opt_proc)) {
    return 1;
  }

  if (conf.verbosity >= 3) {
    spdlog::set_level(spdlog::level::trace);
  } else if (conf.verbosity == 2) {
    spdlog::set_level(spdlog::level::debug);
  } else if (conf.verbosity == 1) {
    spdlog::set_level(spdlog::level::info);
  } else {
    spdlog::set_level(spdlog::level::warn);
  }

  auto metadataDir = std::string(conf.metadata_dir);
  auto metadataDirDRAM = metadataDir + "_tmp";

  std::unique_ptr<Server> server = nullptr;

  if (!conf.in_memory) {
    SnapAction fpga(Card{conf.card, conf.timeout});
    auto factory =
        std::make_shared<OperatorFactory>(OperatorFactory::fromFPGA(fpga));

    std::set<std::string> operators;
    for (const auto &op : factory->operatorSpecifications()) {
      spdlog::info("Found operator {}", op.first);
      operators.emplace(op.first);
    }

    operators.emplace(DatagenOperator::id());
    operators.emplace(MetalCatOperator::id());

    server = std::make_unique<Server>(factory);

    Context::addHandler("/.hello", std::make_unique<SocketFuseHandler>(
                                       server->socketFilename()));
    Context::addHandler("/operators", std::make_unique<OperatorFuseHandler>(
                                          std::move(operators)));

    auto dramFilesystem = std::make_shared<PipelineStorage>(
        Card{conf.card, conf.timeout}, fpga::AddressType::CardDRAM,
        fpga::MapType::DRAM, metadataDirDRAM, true);
    Context::addHandler(
        "/tmp", std::make_unique<FilesystemFuseHandler>(dramFilesystem));

    auto nvmeFilesystem = std::make_shared<PipelineStorage>(
        Card{conf.card, conf.timeout}, fpga::AddressType::NVMe,
        fpga::MapType::DRAMAndNVMe, metadataDir, false, dramFilesystem);
    Context::addHandler(
        "/files", std::make_unique<FilesystemFuseHandler>(nvmeFilesystem));
  } else {
    auto inMemoryFilesystem =
        std::make_shared<InMemoryFilesystem>(metadataDir, true);
    Context::addHandler(
        "/files", std::make_unique<FilesystemFuseHandler>(inMemoryFilesystem));
  }

  spdlog::info("Starting FUSE driver...");
  int retc;

  if (server != nullptr) {
    std::thread serverThread(&Server::start, server.get(), Card{conf.card, conf.timeout});
    retc = fuse_main(args.argc, args.argv, &Context::fuseOperations(), nullptr);
    serverThread.join();
  } else {
    retc = fuse_main(args.argc, args.argv, &Context::fuseOperations(), nullptr);
  }

  return retc;
}
