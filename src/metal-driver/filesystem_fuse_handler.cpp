#include "filesystem_fuse_handler.hpp"

#include <cstring>

extern "C" {
#include <metal-filesystem/inode.h>
#include <metal-filesystem/metal.h>
}

#include <metal-filesystem-pipeline/filesystem_context.hpp>
#include <metal-filesystem-pipeline/metal_pipeline_storage.hpp>

namespace metal {

FilesystemFuseHandler::FilesystemFuseHandler(
    std::shared_ptr<FilesystemContext> filesystem)
    : _filesystem(filesystem) {}

int FilesystemFuseHandler::fuse_chown(const std::string path, uid_t uid,
                                      gid_t gid) {
  return mtl_chown(_filesystem->context(), path.c_str(), uid, gid);
}

int FilesystemFuseHandler::fuse_getattr(const std::string path,
                                        struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));

  if (path.empty()) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  mtl_inode inode;

  int res = mtl_get_inode(_filesystem->context(), path.c_str(), &inode);
  if (res != MTL_SUCCESS) {
    return -res;
  }

  (void)stbuf->st_dev;  // ignored by FUSE
  (void)stbuf->st_ino;  // TODO: inode-ID
  stbuf->st_mode = inode.mode;
  stbuf->st_nlink = 2;  // number of hard links to file.

  stbuf->st_uid = inode.user;   // user-ID of owner
  stbuf->st_gid = inode.group;  // group-ID of owner
  (void)stbuf->st_rdev;  // unused, since this field is meant for special files
                         // which we do not have in our FS
  stbuf->st_size = inode.length;  // length of referenced file in byte
  (void)stbuf->st_blksize;        // ignored by FUSE
  stbuf->st_blocks =
      inode.length / 512 +
      (inode.length % 512 > 0 ? 1
                              : 0);  // number of 512B blocks belonging to file.
  stbuf->st_atime = inode.accessed;  // time of last read or write
  stbuf->st_mtime = inode.modified;  // time of last write
  stbuf->st_ctime =
      inode.created;  // time of last change to either content or inode-data

  return 0;
}

int FilesystemFuseHandler::fuse_readdir(const std::string path, void *buf,
                                        fuse_fill_dir_t filler, off_t offset,
                                        struct fuse_file_info *fi) {
  mtl_dir *dir;

  int res = mtl_opendir(_filesystem->context(), path.c_str(), &dir);

  if (res != MTL_SUCCESS) {
    return -res;
  }

  char current_filename[FILENAME_MAX];
  int readdir_status;
  while ((readdir_status =
              mtl_readdir(_filesystem->context(), dir, current_filename,
                          sizeof(current_filename))) != MTL_COMPLETE) {
    filler(buf, current_filename, NULL, 0);
  }

  return 0;
}

int FilesystemFuseHandler::fuse_create(const std::string path, mode_t mode,
                                       struct fuse_file_info *fi) {
  uint64_t inode_id;
  int res = mtl_create(_filesystem->context(), path.c_str(), mode, &inode_id);
  if (res != MTL_SUCCESS) {
    return -res;
  }

  fi->fh = inode_id;

  return 0;
}

int FilesystemFuseHandler::fuse_readlink(const std::string path, char *buf,
                                         size_t size) {
  return -ENOENT;
}

int FilesystemFuseHandler::fuse_open(const std::string path,
                                     struct fuse_file_info *fi) {
  uint64_t inode_id;
  int res = mtl_open(_filesystem->context(), path.c_str(), &inode_id);

  if (res != MTL_SUCCESS) return -res;

  fi->fh = inode_id;

  return 0;
}

int FilesystemFuseHandler::fuse_read(const std::string path, char *buf,
                                     size_t size, off_t offset,
                                     struct fuse_file_info *fi) {
  int res;

  // TODO: It would be nice if this would work within a single transaction
  uint64_t inode_id;
  res = mtl_open(_filesystem->context(), path.c_str(), &inode_id);
  if (res != MTL_SUCCESS) return -res;

  return static_cast<int>(
      mtl_read(_filesystem->context(), inode_id, buf, size, offset));
}

int FilesystemFuseHandler::fuse_release(const std::string path,
                                        struct fuse_file_info *fi) {
  return 0;
}

int FilesystemFuseHandler::fuse_truncate(const std::string path, off_t size) {
  uint64_t inode_id;
  int res = mtl_open(_filesystem->context(), path.c_str(), &inode_id);

  if (res != MTL_SUCCESS) return -res;

  res = mtl_truncate(_filesystem->context(), inode_id, size);

  if (res != MTL_SUCCESS) return -res;

  return 0;
}

int FilesystemFuseHandler::fuse_write(const std::string path, const char *buf,
                                      size_t size, off_t offset,
                                      struct fuse_file_info *fi) {
  if (fi->fh != 0) {
    mtl_write(_filesystem->context(), fi->fh, buf, size, offset);

    // TODO: Return the actual length that was written (to be returned from
    // mtl_write)
    return size;
  }

  return -ENOSYS;
}

int FilesystemFuseHandler::fuse_unlink(const std::string path) {
  int res = mtl_unlink(_filesystem->context(), path.c_str());

  if (res != MTL_SUCCESS) return -res;

  return 0;
}

int FilesystemFuseHandler::fuse_mkdir(const std::string path, mode_t mode) {
  int res = mtl_mkdir(_filesystem->context(), path.c_str(), mode);

  if (res != MTL_SUCCESS) return -res;

  return 0;
}

int FilesystemFuseHandler::fuse_rmdir(const std::string path) {
  int res = mtl_rmdir(_filesystem->context(), path.c_str());

  if (res != MTL_SUCCESS) return -res;

  return 0;
}

int FilesystemFuseHandler::fuse_rename(const std::string from_path,
                                       const std::string to_path) {
  int res =
      mtl_rename(_filesystem->context(), from_path.c_str(), to_path.c_str());

  if (res != MTL_SUCCESS) return -res;

  return 0;
}

}  // namespace metal
