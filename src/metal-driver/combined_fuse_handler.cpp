#include "combined_fuse_handler.hpp"

#include <cstring>

namespace metal {

int CombinedFuseHandler::fuse_chown(const std::string path, uid_t uid,
                                    gid_t gid) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_chown(subpath, uid, gid);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_getattr(const std::string path,
                                      struct stat *stbuf) {
  if (path == "/") {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_getattr(subpath, stbuf);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_readdir(const std::string path, void *buf,
                                      fuse_fill_dir_t filler, off_t offset,
                                      struct fuse_file_info *fi) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    if (subpath.empty()) subpath = "/";
    return handler.second->fuse_readdir(subpath, buf, filler, offset, fi);
  }

  if (path == "/") {
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (auto &[prefix, handler] : _handlers) {
      (void)handler;
      filler(buf, prefix.c_str() + 1 /* skip leading '/' */, NULL, 0);
    }
  }

  return 0;
}

int CombinedFuseHandler::fuse_create(const std::string path, mode_t mode,
                                     struct fuse_file_info *fi) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_create(subpath, mode, fi);
  }

  return -ENOSYS;
}

int CombinedFuseHandler::fuse_readlink(const std::string path, char *buf,
                                       size_t size) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_readlink(subpath, buf, size);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_open(const std::string path,
                                   struct fuse_file_info *fi) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_open(subpath, fi);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_read(const std::string path, char *buf,
                                   size_t size, off_t offset,
                                   struct fuse_file_info *fi) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_read(subpath, buf, size, offset, fi);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_release(const std::string path,
                                      struct fuse_file_info *fi) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_release(subpath, fi);
  }

  return -ENOSYS;
}

int CombinedFuseHandler::fuse_truncate(const std::string path, off_t size) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_truncate(subpath, size);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_write(const std::string path, const char *buf,
                                    size_t size, off_t offset,
                                    struct fuse_file_info *fi) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_write(subpath, buf, size, offset, fi);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_unlink(const std::string path) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_unlink(subpath);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_mkdir(const std::string path, mode_t mode) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_mkdir(subpath, mode);
  }

  return -ENOSYS;
}

int CombinedFuseHandler::fuse_rmdir(const std::string path) {
  for (const auto &handler : _handlers) {
    if (path.rfind(handler.first, 0) != 0) continue;

    // path starts with handler.first
    auto subpath = path.substr(handler.first.size());
    return handler.second->fuse_rmdir(subpath);
  }

  return -ENOENT;
}

int CombinedFuseHandler::fuse_rename(const std::string from_path,
                                     const std::string to_path) {
  for (const auto &handler : _handlers) {
    if (from_path.rfind(handler.first, 0) != 0) continue;

    if (to_path.rfind(handler.first, 0) != 0)
      return -ENOSYS;  // Cannot move files across sub-handlers

    auto from_subpath = from_path.substr(handler.first.size());
    auto to_subpath = to_path.substr(handler.first.size());
    return handler.second->fuse_rename(from_subpath, to_subpath);
  }

  return -ENOENT;
}

void CombinedFuseHandler::addHandler(std::string prefix,
                                     std::unique_ptr<FuseHandler> handler) {
  _handlers.emplace(std::make_pair(prefix, std::move(handler)));
}

}  // namespace metal
