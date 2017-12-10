#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MTL_SUCCESS 0
#define MTL_ERROR_NOENTRY 2
#define MTL_ERROR_EXISTS 17
#define MTL_ERROR_NOTDIRECTORY 20
#define MTL_ERROR_INVALID_ARGUMENT 22

int mtl_initialize(const char* metadata_store);
int mtl_deinitialize();

int mtl_open(const char* filename);
int mtl_mkdir(const char* filename);
int mtl_create(const char* filename);

#ifdef __cplusplus
}
#endif
