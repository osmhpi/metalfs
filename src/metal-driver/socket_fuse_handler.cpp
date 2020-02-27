#include "socket_fuse_handler.hpp"

#include <cstring>
#include <stdexcept>

namespace metal {

SocketFuseHandler::SocketFuseHandler() {
  // Set a file name for the server socket
  _socket_alias = ".hello";

  char socket_filename[255];
  char socket_dir[] = "/tmp/metal-socket-XXXXXX";
  if (mkdtemp(socket_dir) == nullptr) {
    throw std::runtime_error("Could not create temporary directory.");
  }
  auto socket_file = std::string(socket_dir) + "/metal.sock";
  strncpy(socket_filename, socket_file.c_str(), 255);
  _socket_name = std::string(socket_filename);
}

int SocketFuseHandler::fuse_chown(const std::string path, uid_t uid,
                                  gid_t gid) {
  (void)path;
  (void)uid;
  (void)gid;
  return -ENOSYS;
}

int SocketFuseHandler::fuse_getattr(const std::string path,
                                    struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));
  if (path == _socket_name) {
    stbuf->st_mode = S_IFLNK | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(_socket_name.c_str());
    return 0;
  }
  return -ENOENT;
}

int SocketFuseHandler::fuse_readdir(const std::string path, void *buf,
                                    fuse_fill_dir_t filler, off_t offset,
                                    struct fuse_file_info *fi) {
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  filler(buf, _socket_alias.c_str(), NULL, 0);

  return 0;
}

int SocketFuseHandler::fuse_create(const std::string path, mode_t mode,
                                   struct fuse_file_info *fi) {
  return -ENOSYS;
}

int SocketFuseHandler::fuse_readlink(const std::string path, char *buf,
                                     size_t size) {
  if (path == "/" + _socket_alias) {
    strncpy(buf, _socket_name.c_str(), size);
    return 0;
  }
  return -ENOENT;
}

int SocketFuseHandler::fuse_open(const std::string path,
                                 struct fuse_file_info *fi) {
  return -ENOSYS;
}

int SocketFuseHandler::fuse_read(const std::string path, char *buf, size_t size,
                                 off_t offset, struct fuse_file_info *fi) {
  return -ENOSYS;
}

int SocketFuseHandler::fuse_release(const std::string path,
                                    struct fuse_file_info *fi) {
  return 0;
}

int SocketFuseHandler::fuse_truncate(const std::string path, off_t size) {
  return -ENOSYS;
}

int SocketFuseHandler::fuse_write(const std::string path, const char *buf,
                                  size_t size, off_t offset,
                                  struct fuse_file_info *fi) {
  return -ENOSYS;
}

int SocketFuseHandler::fuse_unlink(const std::string path) { return -ENOSYS; }

int SocketFuseHandler::fuse_mkdir(const std::string path, mode_t mode) {
  return -ENOSYS;
}

int SocketFuseHandler::fuse_rmdir(const std::string path) { return -ENOSYS; }

int SocketFuseHandler::fuse_rename(const std::string from_path,
                                   const std::string to_path) {
  return -ENOSYS;
}

}  // namespace metal
