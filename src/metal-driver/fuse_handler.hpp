#pragma once

#define FUSE_USE_VERSION 31
#define _XOPEN_SOURCE 700

extern "C" {
#include <fuse.h>
}

#include <string>

namespace metal {

class FuseHandler {
 public:
  virtual int fuse_chown(const std::string path, uid_t uid, gid_t gid) = 0;
  virtual int fuse_create(const std::string path, mode_t mode,
                          struct fuse_file_info *fi) = 0;
  virtual int fuse_getattr(const std::string path, struct stat *stbuf) = 0;
  virtual int fuse_mkdir(const std::string path, mode_t mode) = 0;
  virtual int fuse_open(const std::string path, struct fuse_file_info *fi) = 0;
  virtual int fuse_read(const std::string path, char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) = 0;
  virtual int fuse_readdir(const std::string path, void *buf,
                           fuse_fill_dir_t filler, off_t offset,
                           struct fuse_file_info *fi) = 0;
  virtual int fuse_readlink(const std::string path, char *buf, size_t size) = 0;
  virtual int fuse_release(const std::string path,
                           struct fuse_file_info *fi) = 0;
  virtual int fuse_rename(const std::string from_path,
                          const std::string to_path) = 0;
  virtual int fuse_rmdir(const std::string path) = 0;
  virtual int fuse_truncate(const std::string path, off_t size) = 0;
  virtual int fuse_unlink(const std::string path) = 0;
  virtual int fuse_write(const std::string path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi) = 0;
};

}  // namespace metal
