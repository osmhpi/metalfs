#pragma once

#include <stdint.h>
#include "storage.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MTL_SUCCESS 0
#define MTL_COMPLETE 1
#define MTL_ERROR_NOENTRY 2
#define MTL_ERROR_EXISTS 17
#define MTL_ERROR_NOTDIRECTORY 20
#define MTL_ERROR_INVALID_ARGUMENT 22
#define MTL_ERROR_NOTEMPTY 39

#define MTL_MAX_EXTENTS 512

typedef struct mtl_context mtl_context;
int mtl_initialize(mtl_context **context, const char *metadata_store,
                   mtl_storage_backend *storage);
int mtl_deinitialize(mtl_context *context);

typedef struct mtl_inode mtl_inode;
struct mtl_dir;
typedef struct mtl_dir mtl_dir;

int mtl_get_inode(mtl_context *context, const char *path, mtl_inode *inode);
int mtl_open(mtl_context *context, const char *filename, uint64_t *inode_id);
int mtl_opendir(mtl_context *context, const char *filename, mtl_dir **dir);
int mtl_readdir(mtl_context *context, mtl_dir *dir, char *buffer,
                uint64_t size);
int mtl_closedir(mtl_context *context, mtl_dir *dir);
int mtl_mkdir(mtl_context *context, const char *filename);
int mtl_rmdir(mtl_context *context, const char *filename);
int mtl_rename(mtl_context *context, const char *from_filename,
               const char *to_filename);
int mtl_chown(mtl_context *context, const char *path, int uid, int gid);
int mtl_create(mtl_context *context, const char *filename, uint64_t *inode_id);
int mtl_write(mtl_context *context, uint64_t inode_id, const char *buffer,
              uint64_t size, uint64_t offset);
uint64_t mtl_read(mtl_context *context, uint64_t inode_id, char *buffer,
                  uint64_t size, uint64_t offset);
int mtl_truncate(mtl_context *context, uint64_t inode_id, uint64_t offset);
int mtl_unlink(mtl_context *context, const char *filename);

int mtl_load_extent_list(mtl_context *context, uint64_t inode_id,
                         mtl_file_extent *extents, uint64_t *extents_length,
                         uint64_t *file_length);

#ifdef __cplusplus
}
#endif
