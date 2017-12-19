#include "base_test.hpp"

#include <ftw.h>

#include "../libmetal/metal.h"

int rm(const char *path, const struct stat *s, int flag, struct FTW *f)
{
    int status;
    int (*rm_func)( const char * );

    switch( flag ) {
    default:     rm_func = unlink; break;
    case FTW_DP: rm_func = rmdir;
    }
    if (status = rm_func( path ), status != 0) {
      // perror( path );
    }
    return status;
}

void MetalTest::SetUp() {
    // Recursively remove
    if (nftw("test_files", rm, FOPEN_MAX, FTW_DEPTH)) {
        // perror("test_files");
    }

    mkdir("test_files", S_IRWXU);
    mkdir("test_files/metadata_store", S_IRWXU);

    mtl_initialize("test_files/metadata_store");
}

void MetalTest::TearDown() {
    mtl_deinitialize();
}
