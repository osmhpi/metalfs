extern "C" {
#include <metal/metal.h>
}

#include "base_test.hpp"

namespace {

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

TEST_F(MetalTest, RemovesADirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir("/foo"));
  EXPECT_EQ(MTL_SUCCESS, mtl_rmdir("/foo"));
}

TEST_F(MetalTest, FailsWhenRemovingNonEmptyDirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir("/foo"));
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir("/foo/bar"));
  EXPECT_EQ(MTL_ERROR_NOTEMPTY, mtl_rmdir("/foo"));
}

TEST_F(MetalTest, CreatesAFile) {
  EXPECT_EQ(MTL_SUCCESS, mtl_create("/hello_world.txt", NULL));
}

TEST_F(MetalTest, OpensACreatedFile) {
  EXPECT_EQ(MTL_SUCCESS, mtl_create("/hello_world.txt", NULL));
  EXPECT_EQ(MTL_SUCCESS, mtl_open("/hello_world.txt", NULL));
}

TEST_F(MetalTest, FailsWhenOpeningNonExistentFile) {
  EXPECT_EQ(MTL_ERROR_NOENTRY, mtl_open("/hello_world.txt", NULL));
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

TEST_F(MetalTest, WritesToAFile) {
  uint64_t inode;
  EXPECT_EQ(MTL_SUCCESS, mtl_create("/hello_world.txt", &inode));
  char *test = "hello world!";
  EXPECT_EQ(MTL_SUCCESS, mtl_write(inode, test, strlen(test) + 1, 0));
}

TEST_F(MetalTest, ReadsWrittenBytes) {
  uint64_t inode;
  EXPECT_EQ(MTL_SUCCESS, mtl_create("/hello_world.txt", &inode));
  char *test = "hello world!";
  EXPECT_EQ(MTL_SUCCESS, mtl_write(inode, test, strlen(test) + 1, 0));

  char output[256];
  EXPECT_EQ(strlen(test) + 1, mtl_read(inode, output, sizeof(output), 0));

  EXPECT_EQ(0, strncmp(test, output, strlen(test) + 1));
}

TEST_F(MetalTest, TruncatesAFile) {
  uint64_t inode;
  EXPECT_EQ(MTL_SUCCESS, mtl_create("/hello_world.txt", &inode));
  char *test = "hello world!";
  EXPECT_EQ(MTL_SUCCESS, mtl_write(inode, test, strlen(test) + 1, 0));

  EXPECT_EQ(MTL_SUCCESS, mtl_truncate(inode, 0));
}

}  // namespace
