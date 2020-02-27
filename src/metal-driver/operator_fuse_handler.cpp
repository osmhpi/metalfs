#include "operator_fuse_handler.hpp"

#include <libgen.h>
#include <unistd.h>
#include <cassert>
#include <cstring>

#include "pseudo_operators.hpp"

namespace metal {

uint8_t PlaceholderBinary[] = {
#include "metal-driver-placeholder.hex"
};

OperatorFuseHandler::OperatorFuseHandler(std::set<std::string> operators)
    : _operators(std::move(operators)) {}

int OperatorFuseHandler::fuse_chown(const std::string path, uid_t uid,
                                    gid_t gid) {
  (void)path;
  (void)uid;
  (void)gid;
  return -ENOSYS;
}

int OperatorFuseHandler::fuse_getattr(const std::string path,
                                      struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));
  stbuf->st_mode = S_IFREG | 0111;
  stbuf->st_uid = 0;
  stbuf->st_gid = 0;
  stbuf->st_size = sizeof(PlaceholderBinary);
  return 0;
}

int OperatorFuseHandler::fuse_readdir(const std::string path, void *buf,
                                      fuse_fill_dir_t filler, off_t offset,
                                      struct fuse_file_info *fi) {
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  for (const auto &op : _operators) {
    filler(buf, op.c_str(), NULL, 0);
  }

  return 0;
}

int OperatorFuseHandler::fuse_create(const std::string path, mode_t mode,
                                     struct fuse_file_info *fi) {
  return -ENOSYS;
}

int OperatorFuseHandler::fuse_readlink(const std::string path, char *buf,
                                       size_t size) {
  return -ENOENT;
}

int OperatorFuseHandler::fuse_open(const std::string path,
                                   struct fuse_file_info *fi) {
  return -ENOSYS;
}

int OperatorFuseHandler::fuse_read(const std::string path, char *buf,
                                   size_t size, off_t offset,
                                   struct fuse_file_info *fi) {
  assert(path.size() > 1 && path[0] == '/');
  auto basename = path.substr(1);

  if (_operators.find(basename) != _operators.cend()) {
    auto readLength = std::min(size, sizeof(PlaceholderBinary) - offset);
    memcpy(buf, PlaceholderBinary + offset, readLength);

    return readLength;
  }

  return -ENOSYS;
}

int OperatorFuseHandler::fuse_release(const std::string path,
                                      struct fuse_file_info *fi) {
  return 0;
}

int OperatorFuseHandler::fuse_truncate(const std::string path, off_t size) {
  return -ENOSYS;
}

int OperatorFuseHandler::fuse_write(const std::string path, const char *buf,
                                    size_t size, off_t offset,
                                    struct fuse_file_info *fi) {
  return -ENOSYS;
}

int OperatorFuseHandler::fuse_unlink(const std::string path) { return -ENOSYS; }

int OperatorFuseHandler::fuse_mkdir(const std::string path, mode_t mode) {
  return -ENOSYS;
}

int OperatorFuseHandler::fuse_rmdir(const std::string path) { return -ENOSYS; }

int OperatorFuseHandler::fuse_rename(const std::string from_path,
                                     const std::string to_path) {
  return -ENOSYS;
}

}  // namespace metal
