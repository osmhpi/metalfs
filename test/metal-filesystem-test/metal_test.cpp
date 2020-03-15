extern "C" {
#include <metal-filesystem/metal.h>
}

#include "base_test.hpp"

namespace {

TEST_F(MetalTest, CreatesADirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir(_context, "/foo"));
}

TEST_F(MetalTest, CreatesANestedDirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir(_context, "/foo"));
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir(_context, "/foo/bar"));
}

TEST_F(MetalTest, FailsWhenCreatingAnExistingDirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir(_context, "/foo"));
  EXPECT_EQ(MTL_ERROR_EXISTS, mtl_mkdir(_context, "/foo"));
}

TEST_F(MetalTest, RemovesADirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir(_context, "/foo"));
  EXPECT_EQ(MTL_SUCCESS, mtl_rmdir(_context, "/foo"));
}

TEST_F(MetalTest, FailsWhenRemovingNonEmptyDirectory) {
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir(_context, "/foo"));
  EXPECT_EQ(MTL_SUCCESS, mtl_mkdir(_context, "/foo/bar"));
  EXPECT_EQ(MTL_ERROR_NOTEMPTY, mtl_rmdir(_context, "/foo"));
}

TEST_F(MetalTest, CreatesAFile) {
  EXPECT_EQ(MTL_SUCCESS, mtl_create(_context, "/hello_world.txt", NULL));
}

TEST_F(MetalTest, OpensACreatedFile) {
  EXPECT_EQ(MTL_SUCCESS, mtl_create(_context, "/hello_world.txt", NULL));
  EXPECT_EQ(MTL_SUCCESS, mtl_open(_context, "/hello_world.txt", NULL));
}

TEST_F(MetalTest, FailsWhenOpeningNonExistentFile) {
  EXPECT_EQ(MTL_ERROR_NOENTRY, mtl_open(_context, "/hello_world.txt", NULL));
}

TEST_F(MetalTest, ListsDirectoryContents) {
  ASSERT_EQ(MTL_SUCCESS, mtl_mkdir(_context, "/foo"));

  mtl_dir *dir;
  ASSERT_EQ(MTL_SUCCESS, mtl_opendir(_context, "/", &dir));

  char current_filename[FILENAME_MAX];

  EXPECT_EQ(MTL_SUCCESS, mtl_readdir(_context, dir, current_filename,
                                     sizeof(current_filename)));
  EXPECT_STREQ(".", current_filename);

  EXPECT_EQ(MTL_SUCCESS, mtl_readdir(_context, dir, current_filename,
                                     sizeof(current_filename)));
  EXPECT_STREQ("..", current_filename);

  EXPECT_EQ(MTL_SUCCESS, mtl_readdir(_context, dir, current_filename,
                                     sizeof(current_filename)));
  EXPECT_STREQ("foo", current_filename);

  EXPECT_EQ(MTL_COMPLETE, mtl_readdir(_context, dir, current_filename,
                                      sizeof(current_filename)));

  EXPECT_EQ(MTL_SUCCESS, mtl_closedir(_context, dir));
}

TEST_F(MetalTest, WritesToAFile) {
  uint64_t inode;
  EXPECT_EQ(MTL_SUCCESS, mtl_create(_context, "/hello_world.txt", &inode));
  std::string test = "hello world!";
  EXPECT_EQ(MTL_SUCCESS,
            mtl_write(_context, inode, test.c_str(), test.size() + 1, 0));
}

TEST_F(MetalTest, ReadsWrittenBytes) {
  uint64_t inode;
  EXPECT_EQ(MTL_SUCCESS, mtl_create(_context, "/hello_world.txt", &inode));
  std::string test = "hello world!";
  EXPECT_EQ(MTL_SUCCESS,
            mtl_write(_context, inode, test.c_str(), test.size() + 1, 0));

  char output[256];
  EXPECT_EQ(test.size() + 1,
            mtl_read(_context, inode, output, sizeof(output), 0));

  EXPECT_EQ(0, strncmp(test.c_str(), output, test.size() + 1));
}

TEST_F(MetalTest, TruncatesAFile) {
  uint64_t inode;
  EXPECT_EQ(MTL_SUCCESS, mtl_create(_context, "/hello_world.txt", &inode));
  std::string test = "hello world!";
  EXPECT_EQ(MTL_SUCCESS,
            mtl_write(_context, inode, test.c_str(), test.size() + 1, 0));

  EXPECT_EQ(MTL_SUCCESS, mtl_truncate(_context, inode, 0));
}

}  // namespace
