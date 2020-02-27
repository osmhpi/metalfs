#pragma once

#include "fuse_handler.hpp"

#include <map>
#include <memory>
#include <string>

namespace metal {

class CombinedFuseHandler : public FuseHandler {
 public:
  int fuse_chown(const std::string path, uid_t uid, gid_t gid) override;
  int fuse_getattr(const std::string path, struct stat *stbuf) override;
  int fuse_readdir(const std::string path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) override;
  int fuse_create(const std::string path, mode_t mode,
                  struct fuse_file_info *fi) override;
  int fuse_readlink(const std::string path, char *buf, size_t size) override;
  int fuse_open(const std::string path, struct fuse_file_info *fi) override;
  int fuse_read(const std::string path, char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) override;
  int fuse_release(const std::string path, struct fuse_file_info *fi) override;
  int fuse_truncate(const std::string path, off_t size) override;
  int fuse_write(const std::string path, const char *buf, size_t size,
                 off_t offset, struct fuse_file_info *fi) override;
  int fuse_unlink(const std::string path) override;
  int fuse_mkdir(const std::string path, mode_t mode) override;
  int fuse_rmdir(const std::string path) override;
  int fuse_rename(const std::string from_path,
                  const std::string to_path) override;

  void addHandler(std::string prefix, std::unique_ptr<FuseHandler> handler);

 protected:
  std::map<std::string, std::unique_ptr<FuseHandler>> _handlers;
};

}  // namespace metal
