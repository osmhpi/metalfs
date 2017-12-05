#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ftw.h>

#include <sys/stat.h>

#include "gtest/gtest.h"

#include "../libmetal/metal.h"

namespace {

int rm(const char *path, const struct stat *s, int flag, struct FTW *f)
{
    int status;
    int (*rm_func)( const char * );

    switch( flag ) {
    default:     rm_func = unlink; break;
    case FTW_DP: rm_func = rmdir;
    }
    if (status = rm_func( path ), status != 0)
        perror( path );
    else
        puts( path );
    return status;
}

class MetalTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    // Recursively remove
    if (nftw("test_files", rm, FOPEN_MAX, FTW_DEPTH))
      perror("test_files");

    mkdir("test_files", S_IRWXU);

    mtl_initialize("test_files/metadata_store.db");
  }

  virtual void TearDown() {
    mtl_deinitialize();
  }
};

TEST_F(MetalTest, OpensAFile) {
  EXPECT_NE(0, mtl_open("hello_world.txt"));
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
