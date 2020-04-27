#define FUSE_USE_VERSION 31
#define _XOPEN_SOURCE 700

extern "C" {
#include <fuse.h>
}

#include <memory>
#include <string>

#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>

#include "combined_fuse_handler.hpp"

namespace metal {

class Context {
 public:
  static void addHandler(std::string prefix,
                         std::unique_ptr<FuseHandler> handler);
  static struct fuse_operations& fuseOperations();
  static std::pair<std::string, std::shared_ptr<FuseHandler>> resolveHandler(const std::string &path);
  protected:
  static void createHandler();
};

}  // namespace metal
