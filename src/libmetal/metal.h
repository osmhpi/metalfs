#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int mtl_initialize(char* metadata_store);
int mtl_deinitialize();
int mtl_open(char* filename);

#ifdef __cplusplus
}
#endif
