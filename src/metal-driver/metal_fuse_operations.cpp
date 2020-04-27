#include "metal_fuse_operations.hpp"

#include <memory>
#include <thread>

#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

#include <metal-pipeline/data_source.hpp>
#include "combined_fuse_handler.hpp"

namespace metal {

static std::unique_ptr<CombinedFuseHandler> handler = nullptr;
static struct fuse_operations ops {};

void Context::createHandler() {
  handler = std::make_unique<CombinedFuseHandler>();

  ops.chown = [](const char *path, uid_t uid, gid_t gid) {
    spdlog::trace("fuse_chown {}", path);
    return handler->fuse_chown(std::string(path), uid, gid);
  };
  ops.create = [](const char *path, mode_t mode, struct fuse_file_info *fi) {
    spdlog::trace("fuse_create {}", path);
    return handler->fuse_create(std::string(path), mode, fi);
  };
  ops.getattr = [](const char *path, struct stat *stbuf) {
    spdlog::trace("fuse_getattr {}", path);
    return handler->fuse_getattr(std::string(path), stbuf);
  };
  ops.mkdir = [](const char *path, mode_t mode) {
    spdlog::trace("fuse_mkdir {}", path);
    return handler->fuse_mkdir(std::string(path), mode);
  };
  ops.open = [](const char *path, struct fuse_file_info *fi) {
    spdlog::trace("fuse_open {}", path);
    return handler->fuse_open(std::string(path), fi);
  };
  ops.read = [](const char *path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) {
    spdlog::trace("fuse_read {}", path);
    return handler->fuse_read(std::string(path), buf, size, offset, fi);
  };
  ops.readdir = [](const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) {
    spdlog::trace("fuse_readdir {}", path);
    return handler->fuse_readdir(std::string(path), buf, filler, offset, fi);
  };
  ops.readlink = [](const char *path, char *buf, size_t size) {
    spdlog::trace("fuse_readlink {}", path);
    return handler->fuse_readlink(std::string(path), buf, size);
  };
  ops.release = [](const char *path, struct fuse_file_info *fi) {
    spdlog::trace("fuse_release {}", path);
    return handler->fuse_release(std::string(path), fi);
  };
  ops.rename = [](const char *from_path, const char *to_path) {
    spdlog::trace("fuse_rename {} {}", from_path, to_path);
    return handler->fuse_rename(std::string(from_path), std::string(to_path));
  };
  ops.rmdir = [](const char *path) {
    spdlog::trace("fuse_rmdir {}", path);
    return handler->fuse_rmdir(std::string(path));
  };
  ops.truncate = [](const char *path, off_t size) {
    spdlog::trace("fuse_truncate {}", path);
    return handler->fuse_truncate(std::string(path), size);
  };
  ops.unlink = [](const char *path) {
    spdlog::trace("fuse_unlink {}", path);
    return handler->fuse_unlink(std::string(path));
  };
  ops.write = [](const char *path, const char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi) {
    spdlog::trace("fuse_write {}", path);
    return handler->fuse_write(std::string(path), buf, size, offset, fi);
  };
}

struct fuse_operations &Context::fuseOperations() {
  if (handler == nullptr) {
    createHandler();
  }

  return ops;
}

void Context::addHandler(std::string prefix, std::unique_ptr<FuseHandler> h) {
  if (handler == nullptr) {
    createHandler();
  }

  handler->addHandler(prefix, std::move(h));
}

std::pair<std::string, std::shared_ptr<FuseHandler>> Context::resolveHandler(const std::string &path) {
  if (handler == nullptr) {
    return std::make_pair("", nullptr);
  }

  return handler->resolveHandler(path);
}

}  // namespace metal
