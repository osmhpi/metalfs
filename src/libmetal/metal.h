#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MTL_SUCCESS 0
#define MTL_ERROR_NOENTRY 2
#define MTL_ERROR_EXISTS 17
#define MTL_ERROR_NOTDIRECTORY 20
#define MTL_ERROR_INVALID_ARGUMENT 22

int mtl_initialize(const char* metadata_store);
int mtl_deinitialize();

struct mtl_dir;
typedef struct mtl_dir mtl_dir;

int mtl_open(const char *filename);
int mtl_opendir(const char *filename, mtl_dir **dir);
char* mtl_readdir(mtl_dir *dir);
int mtl_mkdir(const char *filename);
int mtl_create(const char *filename);
int mtl_write(uint64_t inode_id, const char *buffer, uint64_t size, uint64_t offset);

#ifdef __cplusplus
}
#endif
