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
#include "../../libmetal/metal.h"

static const char *agent_filepath = "./afu_agent";

static const char *afus_dir = "afus";
static const char *files_dir = "files";
static const char *socket_alias = ".hello";
static char socket_filename[255];


static int chown_callback(const char *path, uid_t uid, gid_t gid,
             struct fuse_file_info *fi)
{
    (void) fi;

    return 0;
}

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

    snprintf(test_filename, FILENAME_MAX, "/%s/", files_dir);
    if (strncmp(path, test_filename, strlen(test_filename)) == 0) {
        if (mtl_open(path + 6) == MTL_SUCCESS) {
            stbuf->st_mode = S_IFREG | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }

        return -ENOENT;
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

    char test_filename[FILENAME_MAX];

    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, socket_alias, NULL, 0);
        filler(buf, afus_dir, NULL, 0);
        filler(buf, files_dir, NULL, 0);
        return 0;
    }

    snprintf(test_filename, FILENAME_MAX, "/%s", afus_dir);
    if (strcmp(path, test_filename) == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        for (size_t i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
            filler(buf, afus[i].name, NULL, 0);
        }
        return 0;
    }

    snprintf(test_filename, FILENAME_MAX, "/%s", files_dir);
    if (strcmp(path, test_filename) == 0) {
        mtl_dir *dir;

        // +6, because path = "/files/<filename>", but we only want "/filename"
        int res;
        if (strlen(path + 6) == 0) {
            res = mtl_opendir("/", &dir);
        } else {
            res = mtl_opendir(path + 6, &dir);
        }

        if (res != MTL_SUCCESS) {
            return -res;
        }

        char current_filename[FILENAME_MAX];
        int readdir_status;
        while ((readdir_status = mtl_readdir(dir, current_filename, sizeof(current_filename))) != MTL_COMPLETE) {
            filler(buf, current_filename, NULL, 0);
        }
    }

    return 0;
}

static int create_callback(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int res;

    char test_filename[FILENAME_MAX];
    snprintf(test_filename, FILENAME_MAX, "/%s", files_dir);
    if (strncmp(path, test_filename, strlen(test_filename)) == 0) {
        res = mtl_create(path + 6);
        if (res != MTL_SUCCESS)
            return -res;

        return 0;
    }

    return -ENOSYS;
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

    int res;

    char test_filename[FILENAME_MAX];
    snprintf(test_filename, FILENAME_MAX, "/%s", files_dir);
    if (strncmp(path, test_filename, strlen(test_filename)) == 0) {
        res = mtl_open(path + 6);

        if (res != MTL_SUCCESS)
            return -res;

        return 0;
    }

    return -ENOSYS;
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

static int truncate_callback(const char *path, off_t size,
            struct fuse_file_info *fi)
{
    char test_filename[FILENAME_MAX];
    snprintf(test_filename, FILENAME_MAX, "/%s/file1", files_dir);
    if (strcmp(path, test_filename) == 0) {
        int res;

        if (fi != NULL && false)
            res = ftruncate(fi->fh, size);
        else
            res = truncate("./test.txt", size);
        if (res == -1)
            return -errno;

        return 0;
    }
    return -ENOENT;
}

static int write_callback(const char *path, const char *buf, size_t size,
        off_t offset, struct fuse_file_info *fi)
{
    char test_filename[FILENAME_MAX];
    snprintf(test_filename, FILENAME_MAX, "/%s/file1", files_dir);
    if (strcmp(path, test_filename) == 0) {
        int fd;
        int res;

        (void) fi;
        if(fi == NULL || true)
            fd = open("./test.txt", O_WRONLY);
        else
            fd = fi->fh;

        if (fd == -1)
            return -errno;

        res = pwrite(fd, buf, size, offset);
        if (res == -1)
            res = -errno;

        if (fi == NULL || true)
            close(fd);
        return res;
    }

    return -ENOENT;
}

static struct fuse_operations fuse_example_operations = {
    .chown = chown_callback,
    .getattr = getattr_callback,
    .open = open_callback,
    .read = read_callback,
    .readdir = readdir_callback,
    .readlink = readlink_callback,
    .release = release_callback,
    .truncate = truncate_callback,
    .write = write_callback,
    .create = create_callback
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

    mtl_initialize("metadata_store");

    return fuse_main(argc, argv, &fuse_example_operations, NULL);
}