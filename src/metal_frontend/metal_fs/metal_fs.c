#define FUSE_USE_VERSION 31

#define _XOPEN_SOURCE 700

#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>

#include <pthread.h>

#include <fuse.h>

#include "../common/known_operators.h"
#include "server.h"
#include "../../metal/metal.h"
#include "../../metal/inode.h"
#include "../../metal_pipeline/pipeline.h"

static char agent_filepath[255];

static const char *afus_dir = "afus";
static const char *files_dir = "files";
static const char *socket_alias = ".hello";
static char socket_filename[255];


static int chown_callback(const char *path, uid_t uid, gid_t gid) {
    printf("path: %s\n", path);
    return mtl_chown(path + 6, uid, gid);
}

static int getattr_callback(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    int res;
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
          mtl_inode inode;

          res = mtl_get_inode(path + 6, &inode);
          if (res != MTL_SUCCESS) {
            return -res;
          }

          stbuf->st_dev; // device ID? can we put something meaningful here?
          stbuf->st_ino; // TODO: inode-ID
          stbuf->st_mode; // set according to filetype below
          stbuf->st_nlink; // number of hard links to file. since we don't support hardlinks as of now, this will always be 0.
          stbuf->st_uid = inode.user; // user-ID of owner
          stbuf->st_gid = inode.group; // group-ID of owner
          stbuf->st_rdev; // unused, since this field is meant for special files which we do not have in our FS
          stbuf->st_size = inode.length; // length of referenced file in byte
          stbuf->st_blksize; // our blocksize. 4k?
          stbuf->st_blocks; // number of 512B blocks belonging to file. TODO: needs to be set in inode whenever we write an extent
          stbuf->st_atime = inode.accessed; // time of last read or write
          stbuf->st_mtime = inode.modified; // time of last write
          stbuf->st_ctime = inode.created; // time of last change to either content or inode-data


          stbuf->st_mode = S_IFREG | 0755;
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

    for (size_t i = 0; i < sizeof(known_operators) / sizeof(known_operators[0]); ++i) {
        snprintf(test_filename, FILENAME_MAX, "/%s/%s", afus_dir, known_operators[i]->name);
        if (strcmp(path, test_filename) != 0) {
            continue;
        }

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
        for (size_t i = 0; i < sizeof(known_operators) / sizeof(known_operators[0]); ++i) {
            filler(buf, known_operators[i]->name, NULL, 0);
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

static int create_callback(const char *path, mode_t mode, struct fuse_file_info *fi) {
    int res;

    char test_filename[FILENAME_MAX];
    snprintf(test_filename, FILENAME_MAX, "/%s", files_dir);
    if (strncmp(path, test_filename, strlen(test_filename)) == 0) {
        uint64_t inode_id;
        res = mtl_create(path + 6, &inode_id);
        if (res != MTL_SUCCESS)
            return -res;

        fi->fh = inode_id;

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

    int res;

    char test_filename[FILENAME_MAX];
    snprintf(test_filename, FILENAME_MAX, "/%s/", files_dir);
    if (strncmp(path, test_filename, strlen(test_filename)) == 0) {
        uint64_t inode_id;
        res = mtl_open(path + 6, &inode_id);

        if (res != MTL_SUCCESS)
            return -res;

        fi->fh = inode_id;

        return 0;
    }

    return -ENOSYS;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

    char test_filename[FILENAME_MAX];

    for (size_t i = 0; i < sizeof(known_operators) / sizeof(known_operators[0]); ++i) {
        snprintf(test_filename, FILENAME_MAX, "/%s/%s", afus_dir, known_operators[i]->name);
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

    if (fi->fh != 0) {
        return mtl_read(fi->fh, buf, size, offset);
    }

    return -ENOSYS;
}

static int release_callback(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

static int truncate_callback(const char *path, off_t size)
{
    // struct fuse_file_info* is not available in truncate :/
    int res;

    char test_filename[FILENAME_MAX];
    snprintf(test_filename, FILENAME_MAX, "/%s", files_dir);
    if (strncmp(path, test_filename, strlen(test_filename)) == 0) {
        uint64_t inode_id;
        res = mtl_open(path + 6, &inode_id);

        if (res != MTL_SUCCESS)
            return -res;

        res = mtl_truncate(inode_id, size);

        if (res != MTL_SUCCESS)
            return -res;

        return 0;
    }

    return -ENOSYS;
}

static int write_callback(const char *path, const char *buf, size_t size,
        off_t offset, struct fuse_file_info *fi)
{
    if (fi->fh != 0) {
        mtl_write(fi->fh, buf, size, offset);

        // TODO: Return the actual length that was written (to be returned from mtl_write)
        return size;
    }

    return -ENOSYS;
}

static int unlink_callback(const char *path) {

    int res;

    char test_filename[FILENAME_MAX];
    snprintf(test_filename, FILENAME_MAX, "/%s/", files_dir);
    if (strncmp(path, test_filename, strlen(test_filename)) == 0) {

        res = mtl_unlink(path + 6);

        if (res != MTL_SUCCESS)
            return -res;

        return 0;
    }

    return -ENOSYS;
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
    .create = create_callback,
    .unlink = unlink_callback
};

int main(int argc, char *argv[])
{
    // Set a file name for the server socket
    char temp[L_tmpnam];
    tmpnam(temp);
    strncpy(socket_filename, temp, 255);

    // Determine the path to the afu_agent executable
    strncpy(agent_filepath, argv[0], sizeof(agent_filepath));
    dirname(agent_filepath);
    strncat(agent_filepath, "/afu_agent", sizeof(agent_filepath));

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, start_socket, (void*)socket_filename);

    mtl_initialize("metadata_store");
    mtl_pipeline_initialize();

    int retc = fuse_main(argc, argv, &fuse_example_operations, NULL);

    // This de-allocates the action/card, so this should definitely be called
    mtl_deinitialize();
    mtl_pipeline_deinitialize();

    return retc;
}
