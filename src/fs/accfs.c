#define FUSE_USE_VERSION 31

#define _XOPEN_SOURCE 700

#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include <fuse.h>

#include "../common/afus.h"
#include "server.h"
#include "afu.h"

static const char *agent_filepath = "./afu_agent";

static const char *afus_dir = "afus";
static const char *files_dir = "files";
static const char *socket_alias = ".hello";
static char socket_filename[255];

static int getattr_callback(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    char test_filename[FILENAME_MAX];

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    snprintf(test_filename, FILENAME_MAX, "/%s", afus_dir);
    if (strcmp(path, test_filename) == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    snprintf(test_filename, FILENAME_MAX, "/%s", files_dir);
    if (strcmp(path, test_filename) == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    snprintf(test_filename, FILENAME_MAX, "/%s", socket_alias);
    if (strcmp(path, test_filename) == 0) {
        stbuf->st_mode = S_IFLNK | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(socket_filename);
        return 0;
    }

    for (size_t i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
        snprintf(test_filename, FILENAME_MAX, "/%s/%s", afus_dir, afus[i].name);
        if (strcmp(path, test_filename) != 0) {
            continue;
        }

        int res;

        res = lstat(agent_filepath, stbuf);
        if (res == -1)
            return -errno;

        return 0;
    }

    snprintf(test_filename, FILENAME_MAX, "/%s/file1", files_dir);
    if (strcmp(path, test_filename) == 0) {
        int res;

        res = lstat("./test.txt", stbuf);
        if (res == -1)
            return -errno;

        return 0;
    }

    return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    char test_filename[FILENAME_MAX];

    if (strcmp(path, "/") == 0) {
        filler(buf, socket_alias, NULL, 0);
        filler(buf, afus_dir, NULL, 0);
        filler(buf, files_dir, NULL, 0);
        return 0;
    }

    snprintf(test_filename, FILENAME_MAX, "/%s", afus_dir);
    if (strcmp(path, test_filename) == 0) {
        for (size_t i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
            filler(buf, afus[i].name, NULL, 0);
        }
        return 0;
    }

    snprintf(test_filename, FILENAME_MAX, "/%s", files_dir);
    if (strcmp(path, test_filename) == 0) {
        filler(buf, "file1", NULL, 0);
        return 0;
    }

    return 0;
}

static int readlink_callback(const char *path, char *buf, size_t size) {
    if (strncmp(path, "/", 1) == 0) {  // probably nonsense
        if (strcmp(path+1, socket_alias) == 0) {
            strncpy(buf, socket_filename, size);
            return 0;
        }
    }

    return -ENOENT;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
    (void)path;
    (void)fi;

    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

    char test_filename[FILENAME_MAX];

    for (size_t i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
        snprintf(test_filename, FILENAME_MAX, "/%s/%s", afus_dir, afus[i].name);
        if (strcmp(path, test_filename) != 0) {
            continue;
        }

        int fd;
        int res;

        if (fi == NULL || true)
            fd = open(agent_filepath, O_RDONLY);
        else
            fd = fi->fh;

        if (fd == -1)
            return -errno;

        res = pread(fd, buf, size, offset);
        if (res == -1)
            res = -errno;

        if(fi == NULL || true)
            close(fd);
        return res;
    }


    snprintf(test_filename, FILENAME_MAX, "/%s/file1", files_dir);
    if (strcmp(path, test_filename) == 0) {
        int fd;
        int res;

        if (fi == NULL || true)
            fd = open("./test.txt", O_RDONLY);
        else
            fd = fi->fh;

        if (fd == -1)
            return -errno;

        res = pread(fd, buf, size, offset);
        if (res == -1)
            res = -errno;

        if(fi == NULL || true)
            close(fd);
        return res;
    }

  return -ENOENT;
}

static int release_callback(const char *path, struct fuse_file_info *fi)
{
	if (strncmp(path, "/", 1) == 0) {  // probably nonsense
        for (size_t i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
            if (strcmp(path+1, afus[i].name) != 0) {
                continue;
            }

            close(fi->fh);
            break;
        }
    }
	return 0;
}

static struct fuse_operations fuse_example_operations = {
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
  .readlink = readlink_callback,
  .release = release_callback
};

int main(int argc, char *argv[])
{
    // Temporarily enable our fake FPGA buffer list
    InitializeListHead(&fpga_buffers);

    // Set a file name for the server socket
    char temp[L_tmpnam];
    tmpnam(temp);
    strncpy(socket_filename, temp, 255);

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, start_socket, (void*)socket_filename);

    return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
