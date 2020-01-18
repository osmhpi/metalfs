#define FUSE_USE_VERSION 31
#define _XOPEN_SOURCE 700

extern "C" {
#include <fuse.h>
}

#include <memory>
#include <string>
#include <unordered_set>

#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>

namespace metal {

class OperatorRegistry;

class Context {
 public:
  static Context &instance();
  void initialize(bool in_memory, std::string bin_path,
                  std::string metadata_dir, int card);

  int card() { return _card; }

  std::string placeholderExecutablePath() { return _agent_filepath; }
  const std::string &socket_alias() { return _socket_alias; }
  std::string socket_name() { return "/" + socket_alias(); }

  const std::string socket_filename() { return _socket_filename; }

  mtl_storage_backend *storage() { return &_storage; }

  const std::string &files_dirname() { return _files_dirname; }
  std::string files_dir() { return "/" + files_dirname(); }
  std::string files_prefix() { return files_dir() + "/"; }

  const std::string &operators_dirname() { return _operators_dirname; }
  std::string operators_dir() { return "/" + operators_dirname(); }
  std::string operators_prefix() { return operators_dir() + "/"; }

  std::shared_ptr<OperatorRegistry> registry() { return _registry; };

  std::unordered_set<std::string>& operators() { return _operators; }

 protected:
  int _card;
  std::shared_ptr<OperatorRegistry> _registry;
  std::string _files_dirname;
  std::string _operators_dirname;
  mtl_storage_backend _storage;
  std::string _agent_filepath;
  std::string _socket_filename;
  std::string _socket_alias;
  std::unordered_set<std::string> _operators;
};

int fuse_chown(const char *path, uid_t uid, gid_t gid);
int fuse_getattr(const char *path, struct stat *stbuf);
int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi);
int fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int fuse_readlink(const char *path, char *buf, size_t size);
int fuse_open(const char *path, struct fuse_file_info *fi);
int fuse_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi);
int fuse_release(const char *path, struct fuse_file_info *fi);
int fuse_truncate(const char *path, off_t size);
int fuse_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi);
int fuse_unlink(const char *path);
int fuse_mkdir(const char *path, mode_t mode);
int fuse_rmdir(const char *path);
int fuse_rename(const char *from_path, const char *to_path);

constexpr struct fuse_operations metal_fuse_operations = [] {
  struct fuse_operations ops {};
  ops.create = fuse_create;
  ops.getattr = fuse_getattr;
  ops.readlink = fuse_readlink;
  ops.readdir = fuse_readdir;
  ops.mkdir = fuse_mkdir;
  ops.unlink = fuse_unlink;
  ops.rmdir = fuse_rmdir;
  ops.rename = fuse_rename;
  ops.chown = fuse_chown;
  ops.truncate = fuse_truncate;
  ops.open = fuse_open;
  ops.read = fuse_read;
  ops.write = fuse_write;
  ops.release = fuse_release;
  return ops;
}();

}  // namespace metal
