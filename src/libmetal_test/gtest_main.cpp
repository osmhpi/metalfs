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
    if (status = rm_func( path ), status != 0) {
      // perror( path );
    }
    return status;
}

class MetalTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    // Recursively remove
    if (nftw("test_files", rm, FOPEN_MAX, FTW_DEPTH)) {
      // perror("test_files");
    }

    mkdir("test_files", S_IRWXU);
    mkdir("test_files/metadata_store", S_IRWXU);

    mtl_initialize("test_files/metadata_store");
  }

  virtual void TearDown() {
    mtl_deinitialize();
  }
};

TEST_F(MetalTest, CreatesADirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir("/foo"));
}

TEST_F(MetalTest, CreatesANestedDirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir("/foo"));
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir("/foo/bar"));
}

TEST_F(MetalTest, FailsWhenCreatingAnExistingDirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir("/foo"));
  EXPECT_EQ(MTL_ERROR_EXISTS, mtl_mkdir("/foo"));
}

TEST_F(MetalTest, CreatesAFile) {
  EXPECT_EQ(MTL_SUCCESS, mtl_create("/hello_world.txt"));
}

TEST_F(MetalTest, OpensACreatedFile) {
  EXPECT_EQ(MTL_SUCCESS, mtl_create("/hello_world.txt"));
  EXPECT_EQ(MTL_SUCCESS, mtl_open("/hello_world.txt"));
}

TEST_F(MetalTest, FailsWhenOpeningNonExistentFile) {
  EXPECT_EQ(MTL_ERROR_NOENTRY, mtl_open("/hello_world.txt"));
}

TEST_F(MetalTest, ListsDirectoryContents) {
  ASSERT_EQ(MTL_SUCCESS, mtl_mkdir("/foo"));

  mtl_dir *dir;
  ASSERT_EQ(MTL_SUCCESS, mtl_opendir("/", &dir));

  char current_filename[FILENAME_MAX];

  EXPECT_EQ(MTL_SUCCESS, mtl_readdir(dir, current_filename, sizeof(current_filename)));
  EXPECT_STREQ(".", current_filename);

  EXPECT_EQ(MTL_SUCCESS, mtl_readdir(dir, current_filename, sizeof(current_filename)));
  EXPECT_STREQ("..", current_filename);

  EXPECT_EQ(MTL_SUCCESS, mtl_readdir(dir, current_filename, sizeof(current_filename)));
  EXPECT_STREQ("foo", current_filename);

  EXPECT_EQ(MTL_COMPLETE, mtl_readdir(dir, current_filename, sizeof(current_filename)));

  EXPECT_EQ(MTL_SUCCESS, mtl_closedir(dir));
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
