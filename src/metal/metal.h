#pragma once

#include <stdint.h>

#define MTL_SUCCESS 0
#define MTL_COMPLETE 1
#define MTL_ERROR_NOENTRY 2
#define MTL_ERROR_EXISTS 17
#define MTL_ERROR_NOTDIRECTORY 20
#define MTL_ERROR_INVALID_ARGUMENT 22
#define MTL_ERROR_NOTEMPTY 39

int mtl_initialize(const char* metadata_store);
int mtl_deinitialize();

typedef struct mtl_inode mtl_inode;
struct mtl_dir;
typedef struct mtl_dir mtl_dir;
typedef struct mtl_file_extent mtl_file_extent;

int mtl_get_inode(const char *path, mtl_inode *inode);
int mtl_open(const char *filename, uint64_t *inode_id);
int mtl_opendir(const char *filename, mtl_dir **dir);
int mtl_readdir(mtl_dir *dir, char *buffer, uint64_t size);
int mtl_closedir(mtl_dir *dir);
int mtl_mkdir(const char *filename);
int mtl_rmdir(const char *filename);
int mtl_rename(const char *from_filename, const char *to_filename);
int mtl_create(const char *filename, uint64_t *inode_id);
int mtl_write(uint64_t inode_id, const char *buffer, uint64_t size, uint64_t offset);
uint64_t mtl_read(uint64_t inode_id, char *buffer, uint64_t size, uint64_t offset);
int mtl_truncate(uint64_t inode_id, uint64_t offset);
int mtl_unlink(const char *filename);

int mtl_prepare_storage_for_reading(const char *filename, uint64_t *size);
int mtl_prepare_storage_for_writing(const char *filename, uint64_t size);
