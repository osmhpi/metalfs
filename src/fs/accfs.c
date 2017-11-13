#define FUSE_USE_VERSION 31

#define _XOPEN_SOURCE 700

#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <fuse.h>

typedef struct afu_entry {
    const char *name;
    const int key;
} afu_entry_t;

static afu_entry_t afus[] = {
    { .name = "blowfish", .key = 0},
    { .name = "sponge"  , .key = 1},
};

static const char *agent_filepath = "./afu_agent";

static int getattr_callback(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strncmp(path, "/", 1) == 0) {  // probably nonsense
        for (int i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
            if (strcmp(path+1, afus[i].name) != 0) {
                continue;
            }

            int res;

            res = lstat(agent_filepath, stbuf);
            if (res == -1)
                return -errno;

            return 0;
        }
    }

    return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (int i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
        filler(buf, afus[i].name, NULL, 0);
    }

    return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {


  if (strncmp(path, "/", 1) == 0) {  // probably nonsense
    for (int i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
        if (strcmp(path+1, afus[i].name) != 0) {
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
  }

  return -ENOENT;
}

static int release_callback(const char *path, struct fuse_file_info *fi)
{
	if (strncmp(path, "/", 1) == 0) {  // probably nonsense
        for (int i = 0; i < sizeof(afus) / sizeof(*afus); ++i) {
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
  .release = release_callback
};

int main(int argc, char *argv[])
{
  return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
